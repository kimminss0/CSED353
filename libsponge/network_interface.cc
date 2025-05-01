#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

using namespace std;

void NetworkInterface::_learn_ethernet_address(uint32_t ip_address, EthernetAddress ethernet_address) {
    // Attempt to insert the sender's IP and expiration time into the lookup table
    auto [it, inserted] =
        _ethernet_address_lookup.emplace(ip_address, std::make_pair(ethernet_address, IP_LOOKUP_EXPIRATION_TIME));

    // If the entry already exists, update the expiration time
    if (!inserted) {
        it->second.second = IP_LOOKUP_EXPIRATION_TIME;
    }

    auto it1 = _address_resolution_queue.find(ip_address);
    if (it1 != _address_resolution_queue.end()) {
        auto &[debounce, pending_datagrams] = it1->second;
        while (!pending_datagrams.empty()) {
            _send_ipv4_frame(ethernet_address, pending_datagrams.front().serialize());
            pending_datagrams.pop();
        }
        _address_resolution_queue.erase(it1);
    }
}

void NetworkInterface::_send_ipv4_frame(EthernetAddress dst, BufferList &&payload) {
    auto frame = EthernetFrame();
    frame.header().dst = dst;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_IPv4;
    frame.payload() = std::move(payload);
    _frames_out.push(std::move(frame));
}

void NetworkInterface::_send_arp_request(uint32_t target_ip_address) {
    auto arp = ARPMessage{};
    arp.opcode = ARPMessage::OPCODE_REQUEST;
    arp.sender_ethernet_address = _ethernet_address;
    arp.sender_ip_address = _ip_address.ipv4_numeric();
    arp.target_ip_address = target_ip_address;

    auto frame = EthernetFrame();
    frame.header().dst = ETHERNET_BROADCAST;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_ARP;
    frame.payload() = arp.serialize();
    _frames_out.push(std::move(frame));
}

void NetworkInterface::_send_arp_reply(uint32_t target_ip_address, EthernetAddress target_ethernet_address) {
    auto arp = ARPMessage{};
    arp.opcode = ARPMessage::OPCODE_REPLY;
    arp.sender_ethernet_address = _ethernet_address;
    arp.sender_ip_address = _ip_address.ipv4_numeric();
    arp.target_ip_address = target_ip_address;
    arp.target_ethernet_address = target_ethernet_address;

    auto frame = EthernetFrame();
    frame.header().dst = target_ethernet_address;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_ARP;
    frame.payload() = arp.serialize();
    _frames_out.push(std::move(frame));
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    auto it = _ethernet_address_lookup.find(next_hop_ip);
    const auto &[eth_addr, expiration_time] = it->second;
    if (it != _ethernet_address_lookup.end() && expiration_time == 0) {
        _ethernet_address_lookup.erase(it);
        it = _ethernet_address_lookup.end();
    }
    if (it == _ethernet_address_lookup.end()) {
        // Queue the datagram for later transmission and send an ARP request if the debounce timer allows
        auto [it1, inserted] = _address_resolution_queue.emplace(
            next_hop_ip, std::make_pair(ARP_DEBOUNCE_TIME, std::queue<InternetDatagram>()));
        auto &[debounce, pending_datagrams] = it1->second;
        pending_datagrams.push(dgram);
        if (inserted || debounce == 0)
            _send_arp_request(next_hop_ip);
        return;
    }
    // Construct an Ethernet frame with the IP datagram and send it to the resolved Ethernet address
    _send_ipv4_frame(eth_addr, dgram.serialize());
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    switch (frame.header().type) {
        case EthernetHeader::TYPE_IPv4: {
            InternetDatagram dgram;
            if (dgram.parse(frame.payload()) != ParseResult::NoError)
                return std::nullopt;
            if (frame.header().dst != _ethernet_address)
                return std::nullopt;
            _learn_ethernet_address(dgram.header().src, frame.header().src);
            return dgram;
        }
        case EthernetHeader::TYPE_ARP: {
            ARPMessage arp;
            if (arp.parse(frame.payload()) != ParseResult::NoError)
                return std::nullopt;

            switch (arp.opcode) {
                case (ARPMessage::OPCODE_REQUEST): {
                    _learn_ethernet_address(arp.sender_ip_address, arp.sender_ethernet_address);
                    // If it is asking for our IP address, send ARP reply
                    if (arp.target_ip_address == _ip_address.ipv4_numeric())
                        _send_arp_reply(arp.sender_ip_address, arp.sender_ethernet_address);
                    return std::nullopt;
                }
                case (ARPMessage::OPCODE_REPLY): {
                    _learn_ethernet_address(arp.sender_ip_address, arp.sender_ethernet_address);
                    _learn_ethernet_address(arp.target_ip_address, arp.target_ethernet_address);
                    return std::nullopt;
                }
                default: {
                    throw std::runtime_error("Unsupported ARP opcode");
                }
            }
        }
        default: {
            return std::nullopt;
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    for (auto &[ip, val] : _address_resolution_queue) {
        auto &[debounce, pending_datagrams] = val;
        debounce -= min(ms_since_last_tick, debounce);
    }
    for (auto &[ip, val] : _ethernet_address_lookup) {
        auto &[eth_addr, expiration_time] = val;
        expiration_time -= min(ms_since_last_tick, expiration_time);
    }
}

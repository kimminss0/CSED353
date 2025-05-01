#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

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
    const auto &[eth_addr, expiration_time] = it->second;
    auto frame = EthernetFrame();
    frame.header().dst = eth_addr;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_IPv4;
    frame.payload() = dgram.serialize();
    _frames_out.push(std::move(frame));
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    DUMMY_CODE(frame);
    return {};
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

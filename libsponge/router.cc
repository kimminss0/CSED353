#include "router.hh"

#include <iostream>

using namespace std;

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";
    _fib.emplace_back(route_prefix, prefix_length, next_hop, interface_num);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    const auto destination = dgram.header().dst;

    if (dgram.header().ttl <= 1)
        return;
    dgram.header().ttl--;

    bool matched = false;
    uint8_t match_len{0};
    optional<Address> next_hop{};
    size_t interface_num;

    for (const auto &v : _fib) {
        uint32_t route_prefix;
        uint8_t prefix_len;
        tie(route_prefix, prefix_len, std::ignore, std::ignore) = v;

        if (matched && prefix_len <= match_len)
            continue;

        const uint32_t netmask = prefix_len > 0 ? (0xffffffff) << (32 - prefix_len) : 0;
        if ((destination & netmask) != route_prefix)
            continue;

        matched = true;
        tie(std::ignore, match_len, next_hop, interface_num) = v;
    }
    if (!matched)
        return;

    interface(interface_num).send_datagram(dgram, next_hop.value_or(Address::from_ipv4_numeric(destination)));
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}

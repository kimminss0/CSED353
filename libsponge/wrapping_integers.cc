#include "wrapping_integers.hh"

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return isn + n; }

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // checkpoint == align + delta
    //               ^^^^^
    //               (1ul << 32) * n'    for some integer n' >= 0
    const uint64_t align = checkpoint & 0xffff'ffff'0000'0000;
    const uint32_t delta = checkpoint & 0x0000'0000'ffff'ffff;
    const uint32_t d = n - isn;

    uint64_t abs_seqno = align + d;
    if (delta > d && (delta - d) > (1u << 31))
        abs_seqno += (1ul << 32);
    else if (delta < d && (d - delta) > (1u << 31) && align != 0)
        abs_seqno -= (1ul << 32);

    return abs_seqno;
}

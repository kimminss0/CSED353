#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!_syn_received) {
        if (seg.header().syn) {
            _syn_received = true;
            _isn = seg.header().seqno;
        } else
            return;
    }
    const auto payload_seqno = seg.header().seqno + (seg.header().syn ? 1 : 0);
    const auto stream_index = unwrap(payload_seqno, _isn, stream_out().bytes_written()) - 1;
    _reassembler.push_substring(std::string{seg.payload().str()}, stream_index, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_syn_received)
        return wrap(1 + stream_out().bytes_written() + (stream_out().input_ended() ? 1 : 0), _isn);
    return std::nullopt;
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }

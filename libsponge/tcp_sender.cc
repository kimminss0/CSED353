#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

void TCPSender::_retransmission_timeout_reached() {
    _segments_out.push(_outstanding_segments.front());
    if (_recv_window)
        _consecutive_retransmissions++;
    _retransmission_timer.reset(_initial_retransmission_timeout << _consecutive_retransmissions);
}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _recv_ackno; }

void TCPSender::fill_window() {
    const auto recv_window1 = std::max(uint16_t{1}, _recv_window);
    while (true) {
        const auto syn_sent = _next_seqno > 0;
        const auto fin_sent = _stream.eof() && _next_seqno == _stream.bytes_written() + 2;
        const auto unused_window = size_t{recv_window1 - bytes_in_flight()};

        if (!unused_window || (syn_sent && _stream.buffer_empty() && !_stream.input_ended()) || fin_sent)
            return;

        TCPSegment seg;
        seg.header().seqno = next_seqno();
        if (!syn_sent) {
            seg.header().syn = true;
        } else {
            const auto data_len = std::min({TCPConfig::MAX_PAYLOAD_SIZE, unused_window, _stream.buffer_size()});
            seg.payload() = _stream.read(data_len);
            seg.header().fin = _stream.eof() && data_len < unused_window;
        }
        _next_seqno += seg.length_in_sequence_space();
        _outstanding_segments.push(seg);
        _segments_out.push(std::move(seg));

        if (!_retransmission_timer.active())
            _retransmission_timer.reset(_initial_retransmission_timeout);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // ignore impossible/old ack
    const auto abs_ackno = unwrap(ackno, _isn, _recv_ackno);
    if (abs_ackno > _next_seqno || abs_ackno < _recv_ackno)
        return;

    // only window size is updated; no need to check outstanding segments
    _recv_window = window_size;
    if (abs_ackno == _recv_ackno)
        return;

    _recv_ackno = abs_ackno;
    while (!_outstanding_segments.empty()) {
        const auto seg = _outstanding_segments.front();
        if (unwrap(seg.header().seqno, _isn, _recv_ackno) + seg.length_in_sequence_space() > _recv_ackno)
            break;
        _outstanding_segments.pop();
    }
    _consecutive_retransmissions = 0;
    _retransmission_timer.reset(_outstanding_segments.empty() ? 0 : _initial_retransmission_timeout);
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { _retransmission_timer.tick(ms_since_last_tick); }

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(std::move(seg));
}

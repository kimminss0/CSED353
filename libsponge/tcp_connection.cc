#include "tcp_connection.hh"

#include <iostream>

using namespace std;

void TCPConnection::_send_rst() {
    _sender.segments_out() = {};
    _segments_out = {};
    _sender.send_empty_segment();
    _sender.segments_out().front().header().rst = true;
    _send_segments();
    _reset_connection();
}

void TCPConnection::_reset_connection() {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
}

void TCPConnection::_send_segments() {
    auto &sender_segments = _sender.segments_out();
    while (!sender_segments.empty()) {
        auto seg = std::move(sender_segments.front());
        sender_segments.pop();
        seg.header().win = std::min(_receiver.window_size(), static_cast<size_t>(numeric_limits<uint16_t>::max()));
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        _segments_out.push(std::move(seg));
    }
}

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_since_last_segment_received = 0;
    if (seg.header().rst)
        _reset_connection();
    else
        _receiver.segment_received(seg);
    if (_receiver.stream_out().eof() && !_sender.stream_in().eof())
        _linger_after_streams_finish = false;
    if (seg.header().ack)
        _sender.ack_received(seg.header().ackno, seg.header().win);

    // respond to keep-alive segment
    if (_receiver.ackno().has_value() && seg.length_in_sequence_space() == 0 &&
        seg.header().seqno == _receiver.ackno().value() - 1)
        _sender.send_empty_segment();

    if (seg.length_in_sequence_space() > 0 && _sender.stream_in().buffer_empty())
        _sender.send_empty_segment();

    _sender.fill_window();
    _send_segments();
}

bool TCPConnection::active() const {
    if (_receiver.stream_out().error() || _sender.stream_in().error())
        return false;
    if (!(_receiver.stream_out().eof() && _sender.stream_in().eof() && !_sender.bytes_in_flight()))
        return true;
    if (!_linger_after_streams_finish)
        return false;
    return _time_since_last_segment_received < 10 * _cfg.rt_timeout;
}

size_t TCPConnection::write(const string &data) {
    const auto written = _sender.stream_in().write(data);
    _sender.fill_window();
    _send_segments();
    return written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS)
        _send_rst();
    _send_segments();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    _send_segments();
}

void TCPConnection::connect() {
    _sender.fill_window();
    _send_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _send_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

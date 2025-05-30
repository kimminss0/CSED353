#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    class Timer {
      private:
        //! \brief callback to be called when the timer expires
        const std::function<void()> _callback;

        //! \brief time remaining on the timer
        size_t _time{0};

      public:
        Timer(std::function<void()> callback) : _callback{callback} {}

        //! \brief Reset and start timer
        //! \param timeout Amount of time to count down; if zero, cancel the timer.
        void reset(const size_t timeout) { _time = timeout; }

        //! \return Whether timer is active or not
        bool active() const { return _time > 0; }

        //! \brief Notifies the timer of the passage of time
        void tick(const size_t ms_since_last_tick) {
            if (_time == 0)
                return;
            if (_time <= ms_since_last_tick) {
                _time = 0;
                _callback();
                return;
            }
            _time -= ms_since_last_tick;
        }
    };

    //! our initial sequence number, the number for our SYN.
    const WrappingInt32 _isn;

    //! \brief current size of receiver window
    uint16_t _recv_window{1};

    //! \brief current absolute acknowledge number of receiver
    uint64_t _recv_ackno{0};

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! \brief the number of consecutive retransmissions
    unsigned int _consecutive_retransmissions{0};

    //! retransmission timeout for the connection
    const unsigned int _initial_retransmission_timeout;

    //! retransmission timer
    Timer _retransmission_timer{[&]() { this->_retransmission_timeout_reached(); }};

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! outstanding segments waiting for acknowledgement
    std::queue<TCPSegment> _outstanding_segments{};

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    //! \brief Retransmission timeout was reached
    void _retransmission_timeout_reached();

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH

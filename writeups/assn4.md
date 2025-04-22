Assignment 4 Writeup
=============

My name: 김민서

My POVIS ID: kimminseo

My student ID (numeric): 20220826

This assignment took me about 7 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Your benchmark results (without reordering, with reordering): [1.42, 1.39]

Program Structure and Design of the TCPConnection:

`TCPConnection` wraps `TCPSender` and `TCPReceiver`. To implement the class, I
defined `_send_segments`, `_send_rst`, and `_reset_connection` helper
functions.

- `bytes_in_flight`, `unassembled_bytes`, `end_input_stream` are trivial
  accessors or modifiers.
- `_time_since_last_segment_received`, `write`, `tick`, `end_input_stream`,
  `connect` involves sending segments: they call `_send_segments` inside the
  logic.
- Clean shutdown is implemented by `active` and `segment_received` methods.
  They utilize `_linger_after_streams_finish` to decide whether to perform
  passive shutdown or to perform lingering.
- Handling RST is performed in `segment_received`, `tick`, and
  `~TCPConnection`. `_send_rst` and `_reset_connection` are the helper
  functions for them. `_send_rst` sends RST packet to its peer, and
  `_reset_connection` sets error state on the bytestreams of TCP sender and
  receiver.

Implementation Challenges:

Implementing `TCPConnection::active()` was challenging. However, after
completing it, the codebase is just 7 lines!

Remaining Bugs:

*No bugs were found.*

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

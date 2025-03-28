Assignment 3 Writeup
=============

My name: 김민서

My POVIS ID: kimminse0

My student ID (numeric): 20220826

This assignment took me about 11 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPSender:

`TCPSender` has methods that are responsible for corresponding events:
`ack_received()` handles receiving an ACK, `tick()` handles the logic to
recognize the passage of time, and `_retransmission_timeout_reached()` handles
the logic for expiring the RTO.

`fill_window()` is regularly called to fill the receiver's window. It attempts
to send segments from the lower sequence number to the maximum allowed sequence
number that fits the current window size. The retransmission timer is reset to
the initial value of RTO.

`ack_received()` updates the information about the receiver's current window.
It releases the outstanding segments holded by the TCP sender whose sequence
number is behind the current acknowledge number of the receiver. If any of the
outstanding segments is acknowledged, it reset the retransmission timer to
the initial value or stop it if all of them were ACKed.

TCP sender possesses an timer to check RTO(retransmission timeout). The design
of the RTO timer is provided by the `TCPSender::Timer` class definition.

`_retransmission_timeout_reached()` is called when RTO expires and retransmits
the oldest-sent segment that has been outstanding. It performs exponential
backoff, i.e., double the value of RTO, and reset the timer to the doubled RTO.

Implementation Challenges:

Designing the public interfaces of `TCPSender::Timer` required some
consideration. I passed a lambda function via the constructor that captures
TCPSender instance and call its method `_retransmission_timeout_reached()`.
Otherwise, I could have implemented in a way that the owner of the timer
instance has a responsibility to manually checks if the timer is expired and
calls the method above itself.

Remaining Bugs:

*No bugs were found.*

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

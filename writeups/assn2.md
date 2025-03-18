Assignment 2 Writeup
=============

My name: 김민서

My POVIS ID: kimminseo
My student ID (numeric): 20220826

This assignment took me about 4 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPReceiver and wrap/unwrap routines:

## `wrap` and `unwrap`

The implementation of `wrap` is simple: just add `isn` and `n`. Since
`WrappingInt32` interprets its internal data as `uint32_t`, the result of
addition is clipped by modulus 2^32.

The implementation of `unwrap` is tricky. It first decompose `checkpoint` into
`align` and `delta`, where `checkpoint = align + data` and `align = (2^32) *
n'` where n' is some positive integer. Then, it calculates `n - isn % 2^32`.
Note that the variable `d` is of type `uint32_t`.

Now we compare the distance of `d` and `delta` with 2^31. What we want to see
here is that `align + d` must be included in the range of (checkpoint - 2^31,
checkpoint + 2^31). If not, add or subtract 2^32 to make it fall within the
range and return the absolute sequence number.

## `TCPReceiver`

The three public methods were implemented: `segment_received`, `ackno`, and
`window_size`. First, take account of `window_size`. What we need to return is
the distance between the first unacceptable byte and the first unassembled
byte. Note that distance is same as the remaining capacity of the byte stream. 

Now we take a look into `ackno` method. If SYN packet has not been received,
then it must return `std::nullopt`. If SYN has once been received, the private
member `_isn`, which indicates to the initial sequence number, is initialized;
hence we can utilize `_isn` within `wrap` function that we have already
implemented. The absolute sequence number is the addition of these three values:
1, "written bytes to the byte stream", and 0/1. The first 'one' corresponds to
SYN. Note that the absolute seqno. of SYN is 0, and `ackno()` must return 1.
The last 0/1 value depends on whether FIN has received or not. Once FIN has
been received, the stream reports that the input is ended. At that case, FIN
counts to the sequence number.

Finally, let's take into account the `segment_received` method. Before SYN has
been received, it drops all segment unless it has SYN flag on its TCP header.
If so, it initialize `_isn` and continues to push the payload into the string
reassembler. To do that, it calculates the sequence number of payload. Note
that `seg.header().seqno` indicates the sequence number of SYN if it has that
flag.  Using that payload sequence number, it calculates the stream index by
unwrapping it using `_isn` and some checkpoint value. In this implement, the
index of the last reassembled byte is used. Finally, if FIN is received, it
marks it as eof.

Implementation Challenges:

It was challenging to implement unwrap function. I need to use my pen and paper
to demonstrate the formula.

Remaining Bugs:

*No bugs were found.*

- Optional: I had unexpected difficulty with: writing this writeup was time consuming

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

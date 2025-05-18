Assignment 6 Writeup
=============

My name: 김민서

My POVIS ID: kimminseo

My student ID (numeric): 20220826

This assignment took me about 1 hour to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the Router:

The router maintains its forwarding table and forwards IP datagrams to the
proper interface by looking up the forwarding table. The forwarding table is
implemented as a vector of tuples containing the route prefix, prefix length,
next hop, and interface number. When routing an IP datagram, prefix matching
occurs, and the longest prefix match is selected. If `next_hop` is nullopt, the
destination address is used instead.

Implementation Challenges:

Implementing prefix matching took some time to implement.

Remaining Bugs:

*No bugs were found.*

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

Assignment 5 Writeup
=============

My name: 김민서

My POVIS ID: kimminseo

My student ID (numeric): 20220826

This assignment took me about 6 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the NetworkInterface:

`NetworkInterface` has two public method for sending IP datagram and receiving
Ethernet frame, and one method for mesure the time spent. To implement them, I
wrote some helper methods: `_learn_ethernet_address`, `_send_ipv4_frame`,
`_send_arp_request`, and `_send_arp_reply`.

To manage the learned IP address to the Ethernet address mapping, it has a map
data structure whose key is an IPv4 address and the value is a pair of the
learned Ethernet address and the expiration time. Also, there is another map to
manage the IP datagrams awaiting resolution of their destination IP address. 

When sending a datagram, it looks up to the internal map to find its corresponding
Ethernet address. If successes, just send the dataframe. If fails, it adds the
dataframe to the queue and send an ARP request, respecting the debounce time of
5 seconds.

When receiving a Ethernet frame, if the frame is type of IPv4, then parses it
and check the destination address of the Ethernet frame. If the address
matches, it learns the src Ethernet address and returns the parsed IP datagram.
If the frame is type of ARP and the OPCODE is request, then it learns the
ethernet address of the sender and sends ARP reply if the target IP address
is ours. If the frame is type of ARP and OPCODE is reply, then it learns the
ethernet address of the sender and target.

Implementation Challenges:

Deciding data structure to store the learned IP addr - Ethernet addr map and
its expiration time, pending IP addr and its queued IP dataframes to be sent
took some time.

Remaining Bugs:

*No bugs were found.*

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

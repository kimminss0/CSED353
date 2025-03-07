Assignment 1 Writeup
=============

My name: 김민서

My POVIS ID: kimminseo

My student ID (numeric): 20220826

This assignment took me about 14 hours to do (including the time on studying, designing, and writing the code).

Program Structure and Design of the StreamReassembler:

The implementation manages unassembled data using the `std::map` data
structure. `std::map` manages key-value pairs and keeps each entry sorted by
the key. It provides an efficient way to search, insert, and removal of data.
In the StreamReassembler implementation, the index of the first byte of data
chunk (substring) is used as key, and the entire substring is used as the
value.

For each operation to push a string to the re-assembler, that string is divided
properly and only portion of them is picked up and inserted to the internal map
data structure to not introduce any duplication of data. After insertion of the
divided substrings, it looks up to the map and attempts to assemble fragmented
data and flush them to the byte stream. If the byte stream is full so that no
more byte is able to be pushed, that bytes are just discarded and that portion
of data must be delivered again by the sender.

Implementation Challenges:

I couldn't make design decisions for some of the situations until trying
provided test cases:

- If the byte stream is full and not able to accept the assembled strings,
  should the re-assembler keep or discard that data?
- etc. (I don't remember more)

Remaining Bugs:

*No bugs were found.*

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by:
  * Convenient debugging experience provided by CTest/CMake

- Optional: I'm not sure about: [describe]

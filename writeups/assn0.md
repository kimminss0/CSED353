Assignment 0 Writeup
=============

My name: 김민서

My POVIS ID: kimminseo

My student ID (numeric): 20220826

This assignment took me about 5 hours to do (including the time on studying, designing, and writing the code).

My secret code from section 2.1 was: 8a1d328ce4

- Optional: I think you could make this lab better by:
  * Ensure that the given skeleton codes pass `make format`, as well as let cmake be configured to use clang-format by default.
  * Fix Typos: at writeup page 8, `libsponge/byte stream.{hh,cc}` -> `libsponge/byte_stream.{hh,cc}` (missing underscore)

- Optional: I was surprised by: well documented assignment guidelines

- Optional: I'm not sure about:
  * [webget.cc] Would it be better to close the tcp socket manually? It will be automatically closed by the destructor anyway.
  * Why do we need to use "\r\n" instead of "\n"? In addition, I tried with "\n" and it also suceeds `make check_webget` without any problem.
  * [byte_stream.hh] `ByteStream::_error` remains unused. I tried to utilize it inside of `ByteStream::peek_output` and `ByteStream::pop_output`, but due to the `const` constraint of the method, I could not manipulate any member variables inside of `peek_output`.
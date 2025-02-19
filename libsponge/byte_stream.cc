#include "byte_stream.hh"

#include <stdexcept>

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _capacity(capacity), _buffer(capacity), _buffer_size(0), _bytes_written(0), _bytes_read(0), _input_ended(false) {}

size_t ByteStream::write(const string &data) {
    const auto write_size = std::min(remaining_capacity(), data.length());
    for (size_t i = 0; i < write_size; i++) {
        const auto write_loc = (bytes_written() + i) % _capacity;
        _buffer[write_loc] = data[i];
    }
    _bytes_written += write_size;
    _buffer_size += write_size;
    return write_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
std::string ByteStream::peek_output(const size_t len) const {
    if (len > buffer_size()) {
        throw std::length_error("Attempted to read more than the buffer size");
    }
    std::string ret;
    ret.reserve(len);
    for (size_t i = 0; i < len; i++) {
        const auto read_loc = (bytes_read() + i) % _capacity;
        ret.push_back(_buffer[read_loc]);
    }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (len > buffer_size()) {
        throw std::length_error("Attempted to pop more than the buffer size");
    }
    _bytes_read += len;
    _buffer_size -= len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    const auto ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() { _input_ended = true; }

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _buffer_size; }

bool ByteStream::buffer_empty() const { return !buffer_size(); }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }

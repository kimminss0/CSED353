#include "stream_reassembler.hh"

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _bytes()
    , _bytes_assembled{}
    , _bytes_unassembled{}
    , _last_byte{}
    , _eof{} {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    // Already pushed to the byte stream
    if (_bytes_assembled > index + data.length())
        return;

    const uint64_t data_begin = index;
    const uint64_t data_unassembled_begin = std::max(data_begin, _bytes_assembled);
    const uint64_t data_end = data_begin + data.length();
    if (eof) {
        _eof = true;
        _last_byte = data_end;
    }

    // Already pushed to the byte stream
    if (data_unassembled_begin == data_end) {
        _check_and_end_input();
        return;
    }

    // Since empty, no need to consider other unassembled data chunks
    if (this->empty()) {
        _insert_bytes(data_unassembled_begin, data.substr(data_unassembled_begin - data_begin));
        _assemble_and_push_bytes();
        _check_and_end_input();
        return;
    }

    auto next = _bytes.upper_bound(data_unassembled_begin);
    decltype(next) it;
    if (next == _bytes.begin()) {
        _insert_bytes(data_unassembled_begin,
                      data.substr(data_unassembled_begin - data_begin, next->first - data_unassembled_begin));
        it = next;
        next = std::next(it);
    } else {
        it = std::prev(next);
    }
    uint64_t begin = std::max(it->first + it->second.length(), data_unassembled_begin);

    while (true) {
        auto end = next->first;
        if (next == _bytes.end() || data_end <= end) {
            if (begin < data_end)
                _insert_bytes(begin, data.substr(begin - data_begin));
            break;
        }
        _insert_bytes(begin, data.substr(begin - data_begin, end - begin));
        it = next;
        next = std::next(it);
        begin = it->first + it->second.length();
    }
    _assemble_and_push_bytes();
    _check_and_end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _bytes_unassembled; }

bool StreamReassembler::empty() const { return _bytes.empty(); }

void StreamReassembler::_insert_bytes(uint64_t index, std::string &&data) {
    if (data.empty())
        return;
    _bytes_unassembled += data.length();
    _bytes[index] = std::move(data);
}

void StreamReassembler::_assemble_and_push_bytes() {
    for (auto it = _bytes.find(_bytes_assembled); it->first == _bytes_assembled && it != _bytes.end();) {
        const auto str = it->second;
        auto sz = _output.write(str);
        _bytes_unassembled -= str.length();
        _bytes_assembled += sz;
        auto t = it;
        it = std::next(it);
        _bytes.erase(t);
    }
}

bool StreamReassembler::_check_and_end_input() {
    auto is_end_of_input = _eof && _bytes_assembled == _last_byte;
    if (is_end_of_input)
        _output.end_input();
    return is_end_of_input;
}
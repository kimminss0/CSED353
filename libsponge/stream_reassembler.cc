#include "stream_reassembler.hh"

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output{capacity}
    , _capacity{capacity}
    , _bytes{}
    , _bytes_assembled{}
    , _bytes_unassembled{}
    , _last_byte{}
    , _eof{} {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    const uint64_t data_begin = index;
    const uint64_t data_end = data_begin + data.length();
    const uint64_t data_unassembled_begin = std::max(data_begin, _bytes_assembled);
    const uint64_t data_end_clipped = std::min(data_end, _bytes_assembled + _output.remaining_capacity());

    if (eof && data_end == data_end_clipped) {
        _eof = true;
        _last_byte = data_end_clipped;
    }

    auto next = _bytes.upper_bound(data_unassembled_begin);
    decltype(next) it;
    uint64_t begin;
    uint64_t end = next->first;

    if (next == _bytes.begin()) {
        begin = data_unassembled_begin;
    } else {
        it = std::prev(next);
        begin = std::max(it->first + it->second.length(), data_unassembled_begin);
    }
    while (begin < data_end_clipped) {
        if (next == _bytes.end()) {
            _insert_bytes(next, begin, data.substr(begin - data_begin));
            break;
        }
        if (end != begin)
            _insert_bytes(next, begin, data.substr(begin - data_begin, end - begin));
        it = next;
        next = std::next(it);
        begin = it->first + it->second.length();
        end = next->first;
    }
    _assemble_and_push_bytes();
    _attempt_end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _bytes_unassembled; }

bool StreamReassembler::empty() const { return _bytes.empty(); }

void StreamReassembler::_insert_bytes(decltype(_bytes)::const_iterator hint, const uint64_t index, std::string &&data) {
    if (data.empty())
        return;
    _bytes_unassembled += data.length();
    _bytes.try_emplace(hint, index, std::move(data));
}

void StreamReassembler::_assemble_and_push_bytes() {
    for (auto it = _bytes.find(_bytes_assembled); it != _bytes.end() && it->first == _bytes_assembled;) {
        const auto str = it->second;
        const auto sz = _output.write(str);
        _bytes_unassembled -= str.length();
        _bytes_assembled += sz;
        const auto t = it;
        it = std::next(it);
        _bytes.erase(t);
    }
}

void StreamReassembler::_attempt_end_input() {
    if (_eof && _bytes_assembled == _last_byte)
        _output.end_input();
}
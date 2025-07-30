#pragma once
#include <Arduino.h>
#include <TLogPlus.h>

class BufferedLogStream : public TLogPlus::LOGBase {
public:
    explicit BufferedLogStream(size_t capacity)
        : _capacity(capacity), _head(0), _size(0), _line(""), _buffer(nullptr) {
        _buffer = new String[_capacity];
    }

    ~BufferedLogStream() {
        delete[] _buffer;
    }

    // TLogPlus will call this
    size_t write(uint8_t c) override {
        if (c == '\n') {
            _buffer[_head] = _line;
            _line = "";
            _head = (_head + 1) % _capacity;
            if (_size < _capacity) _size++;
        } else {
            _line += (char)c;
        }
        return 1;
    }

    size_t write(const uint8_t *buffer, size_t size) override {
        size_t written = 0;
        for (size_t i = 0; i < size; ++i) {
            written += write(buffer[i]);
        }
        return written;
    }

    void printAll(Print& out) const {
        for (size_t i = 0; i < _size; ++i) {
            size_t index = (_head + _capacity - _size + i) % _capacity;
            out.print(" > ");
            out.println(_buffer[index]);
        }
    }

    void clear() {
        for (size_t i = 0; i < _capacity; ++i) {
            _buffer[i] = "";
        }
        _line = "";
        _head = 0;
        _size = 0;
    }

    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }

private:
    size_t _capacity;
    size_t _head;
    size_t _size;
    String _line;
    String* _buffer;
};
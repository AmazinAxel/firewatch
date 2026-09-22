// Host stub of Arduino's Print for tools/preview.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

class Print {
 public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t* buffer, size_t size) {
    size_t n = 0;
    while (size--) n += write(*buffer++);
    return n;
  }
  size_t print(const char* s) {
    return write(reinterpret_cast<const uint8_t*>(s), strlen(s));
  }
  size_t print(char c) { return write(static_cast<uint8_t>(c)); }
};

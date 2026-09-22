// Minimal Arduino stub so the real renderer + Adafruit_GFX compile on the
// host for tools/preview (PNG snapshots of the dashboard). Not used on
// device.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#define PROGMEM
#define PGM_P const char*
#define pgm_read_byte(addr) (*(const unsigned char*)(addr))
#define pgm_read_word(addr) (*(const unsigned short*)(addr))
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))

#define radians(deg) ((deg)*0.017453292519943295)
#define degrees(rad) ((rad)*57.29577951308232)

typedef bool boolean;
typedef uint8_t byte;

inline void yield() {}
inline unsigned long micros() { return 0; }
inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}

class __FlashStringHelper;
#define F(x) (x)

class String : public std::string {
 public:
  using std::string::string;
  String(const std::string& s) : std::string(s) {}
};

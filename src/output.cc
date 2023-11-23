#include "wamon/output.h"

namespace wamon {

void byte_to_string(unsigned char c, char (&buf)[4]) {
  buf[0] = '0';
  buf[1] = 'X';
  int h = static_cast<int>(c) / 16;
  int l = static_cast<int>(c) - h * 16;
  if (h >= 0 && h <= 9) {
    buf[2] = h + '0';
  } else {
    buf[2] = h - 10 + 'A';
  }
  if (l >= 0 && l <= 9) {
    buf[3] = l + '0';
  } else {
    buf[3] = l - 10 + 'A';
  }
}

}  // namespace wamon
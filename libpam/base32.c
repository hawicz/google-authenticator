#include <stdio.h>
// Base32 implementation
//
// Copyright 2010 Google Inc.
// Author: Markus Gutschke
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>

#include "base32.h"

int base32_decode(const uint8_t *encoded, uint8_t *result, int bufSize) {
  unsigned int buffer = 0;
  int bitsLeft = 0;
  int count = 0;
  int pad = 0;
  for (const uint8_t *ptr = encoded; count < bufSize && *ptr; ++ptr) {
    uint8_t ch = *ptr;
    if (ch == '=') {
      pad = 1;
      bitsLeft += 5;
      if (bitsLeft >= 8) {
        bitsLeft -= 8;
      }
      if (bitsLeft == 0) {
        pad = 0;
      }
      continue;
    }
    if (pad) {
      // In pad handling mode, but we didn't get a "="
      return -1;
    }
    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') {
      continue;
    }
    buffer <<= 5;

    // Deal with commonly mistyped characters
    if (ch == '0') {
      ch = 'O';
    } else if (ch == '1') {
      ch = 'L';
    } else if (ch == '8') {
      ch = 'B';
    }

    // Look up one base32 digit
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
      ch = (ch & 0x1F) - 1;
    } else if (ch >= '2' && ch <= '7') {
      ch -= '2' - 26;
    } else {
      return -1;
    }

    buffer |= ch;
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      result[count++] = buffer >> (bitsLeft - 8);
      bitsLeft -= 8;
    }
  }
  if (count < bufSize) {
    result[count] = '\000';
  }
  return count;
}

/*
 * Encode <length> bytes of data from <data> into the buffer <result> of
 * length <bufSize>.  <result> is always nul terminated, unless bufSize is 0.
 * If <pad> is non zero, include '=' characters at the end to ensure the ouput
 * length is a multiple of 8 characters.
 *
 * Returns the number of bytes of <result> used, not including terminating nul.
 * If there is not enough room in <result>, or if the input length is less
 * than zero, then -1 is returned.
 */
int base32_encode(const uint8_t *data, int length, uint8_t *result,
                  int bufSize, int pad) {
  if (length < 0 || length > (1 << 28) || bufSize <= 0) {
    return -1;
  }
  int count = 0;
  if (length > 0) {
    unsigned int buffer = data[0];
    int next = 1;
    int bitsLeft = 8;
    while (count < bufSize && (bitsLeft > 0 || next < length)) {
      if (bitsLeft < 5) {
        if (next < length) {
          buffer <<= 8;
          buffer |= data[next++] & 0xFF;
          bitsLeft += 8;
        } else {
          int pad = 5 - bitsLeft;
          buffer <<= pad;
          bitsLeft += pad;
        }
      }
      int index = 0x1F & (buffer >> (bitsLeft - 5));
      bitsLeft -= 5;
      result[count++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"[index];
    }
  }
  while (pad && count < bufSize && count % 8 != 0)
  {
    result[count] = '=';
    count++;
  }
  if (count >= bufSize) {
    // We're out of room, so return an error, but leave as much encoded
    // data as we were able to generate, zero terminated for safety.
    result[bufSize - 1] = '\0';
    return -1;
  }
  result[count] = '\000';
  return count;
}

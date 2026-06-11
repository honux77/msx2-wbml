// NEC MC-8123 decryption (C port of MAME mc8123.cpp / tools/mc8123.py).
#ifndef MC8123_H_
#define MC8123_H_

#include <stdint.h>

// Decode `length` bytes of `rom` into separate opcode and data buffers using
// the 0x2000-byte `key`. opcodes/data must each hold `length` bytes.
// Region offsets >= 0xC000 decrypt as if mapped at 0x8000 (banked ROM).
void mc8123_decode(const uint8_t *rom, const uint8_t *key,
                   uint8_t *opcodes, uint8_t *data, unsigned length);

#endif // MC8123_H_

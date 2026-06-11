// NEC MC-8123 decryption — C port of MAME src/devices/cpu/z80/mc8123.cpp.
// Algorithm by Nicola Salmoria, tables by David Widel (BSD-3-Clause).
// Mirrors tools/mc8123.py, which was validated against the wbml ROMs.
#include "mc8123.h"

#define BIT(v, n) (((v) >> (n)) & 1)

// bitswap<8>(val, b7..b0): result bit (7-i) = val bit args[i].
static uint8_t bitswap8(uint8_t v, int b7, int b6, int b5, int b4,
                        int b3, int b2, int b1, int b0) {
    return (uint8_t)(
        (BIT(v, b7) << 7) | (BIT(v, b6) << 6) | (BIT(v, b5) << 5) |
        (BIT(v, b4) << 4) | (BIT(v, b3) << 3) | (BIT(v, b2) << 2) |
        (BIT(v, b1) << 1) | (BIT(v, b0) << 0));
}

static uint8_t type0(uint8_t v, uint8_t p, unsigned s) {
    if (s == 0) v = bitswap8(v, 7, 5, 3, 1, 2, 0, 6, 4);
    if (s == 1) v = bitswap8(v, 5, 3, 7, 2, 1, 0, 4, 6);
    if (s == 2) v = bitswap8(v, 0, 3, 4, 6, 7, 1, 5, 2);
    if (s == 3) v = bitswap8(v, 0, 7, 3, 2, 6, 4, 1, 5);
    if (BIT(p, 3) && BIT(v, 7)) v ^= (1 << 5) | (1 << 3) | (1 << 0);
    if (BIT(p, 2) && BIT(v, 6)) v ^= (1 << 7) | (1 << 2) | (1 << 1);
    if (BIT(v, 6)) v ^= (1 << 7);
    if (BIT(p, 1) && BIT(v, 7)) v ^= (1 << 6);
    if (BIT(v, 2)) v ^= (1 << 5) | (1 << 0);
    v ^= (1 << 4) | (1 << 3) | (1 << 1);
    if (BIT(p, 2)) v ^= (1 << 5) | (1 << 2) | (1 << 0);
    if (BIT(p, 1)) v ^= (1 << 7) | (1 << 6);
    if (BIT(p, 0)) v ^= (1 << 5) | (1 << 0);
    if (BIT(p, 0)) v = bitswap8(v, 7, 6, 5, 1, 4, 3, 2, 0);
    return v;
}

static uint8_t type1a(uint8_t v, uint8_t p, unsigned s) {
    if (s == 0) v = bitswap8(v, 4, 2, 6, 5, 3, 7, 1, 0);
    if (s == 1) v = bitswap8(v, 6, 0, 5, 4, 3, 2, 1, 7);
    if (s == 2) v = bitswap8(v, 2, 3, 6, 1, 4, 0, 7, 5);
    if (s == 3) v = bitswap8(v, 6, 5, 1, 3, 2, 7, 0, 4);
    if (BIT(p, 2)) v = bitswap8(v, 7, 6, 1, 5, 3, 2, 4, 0);
    if (BIT(v, 1)) v ^= (1 << 0);
    if (BIT(v, 6)) v ^= (1 << 3);
    if (BIT(v, 7)) v ^= (1 << 6) | (1 << 3);
    if (BIT(v, 2)) v ^= (1 << 6) | (1 << 3) | (1 << 1);
    if (BIT(v, 4)) v ^= (1 << 7) | (1 << 6) | (1 << 2);
    if (BIT(v, 7) ^ BIT(v, 2)) v ^= (1 << 4);
    v ^= (1 << 6) | (1 << 3) | (1 << 1) | (1 << 0);
    if (BIT(p, 3)) v ^= (1 << 7) | (1 << 2);
    if (BIT(p, 1)) v ^= (1 << 6) | (1 << 3);
    if (BIT(p, 0)) v = bitswap8(v, 7, 6, 1, 4, 3, 2, 5, 0);
    return v;
}

static uint8_t type1b(uint8_t v, uint8_t p, unsigned s) {
    if (s == 0) v = bitswap8(v, 1, 0, 3, 2, 5, 6, 4, 7);
    if (s == 1) v = bitswap8(v, 2, 0, 5, 1, 7, 4, 6, 3);
    if (s == 2) v = bitswap8(v, 6, 4, 7, 2, 0, 5, 1, 3);
    if (s == 3) v = bitswap8(v, 7, 1, 3, 6, 0, 2, 5, 4);
    if (BIT(v, 2) && BIT(v, 0)) v ^= (1 << 7) | (1 << 4);
    if (BIT(v, 7)) v ^= (1 << 2);
    if (BIT(v, 5)) v ^= (1 << 7) | (1 << 2);
    if (BIT(v, 1)) v ^= (1 << 5);
    if (BIT(v, 6)) v ^= (1 << 1);
    if (BIT(v, 4)) v ^= (1 << 6) | (1 << 5);
    if (BIT(v, 0)) v ^= (1 << 6) | (1 << 2) | (1 << 1);
    if (BIT(v, 3)) v ^= (1 << 7) | (1 << 6) | (1 << 2) | (1 << 1) | (1 << 0);
    v ^= (1 << 6) | (1 << 4) | (1 << 0);
    if (BIT(p, 3)) v ^= (1 << 4) | (1 << 1);
    if (BIT(p, 2)) v ^= (1 << 7) | (1 << 6) | (1 << 3) | (1 << 0);
    if (BIT(p, 1)) v ^= (1 << 4) | (1 << 3);
    if (BIT(p, 0)) v ^= (1 << 6) | (1 << 2) | (1 << 1) | (1 << 0);
    return v;
}

static uint8_t type2a(uint8_t v, uint8_t p, unsigned s) {
    if (s == 0) v = bitswap8(v, 0, 1, 4, 3, 5, 6, 2, 7);
    if (s == 1) v = bitswap8(v, 6, 3, 0, 5, 7, 4, 1, 2);
    if (s == 2) v = bitswap8(v, 1, 6, 4, 5, 0, 3, 7, 2);
    if (s == 3) v = bitswap8(v, 4, 6, 7, 5, 2, 3, 1, 0);
    if (BIT(v, 3) || (BIT(p, 1) && BIT(v, 2)))
        v = bitswap8(v, 6, 0, 7, 4, 3, 2, 1, 5);
    if (BIT(v, 5)) v ^= (1 << 7);
    if (BIT(v, 6)) v ^= (1 << 5);
    if (BIT(v, 0)) v ^= (1 << 6);
    if (BIT(v, 4)) v ^= (1 << 3) | (1 << 0);
    if (BIT(v, 1)) v ^= (1 << 2);
    v ^= (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 1);
    if (BIT(p, 2)) v ^= (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0);
    if (BIT(p, 3)) {
        if (BIT(p, 0)) v = bitswap8(v, 7, 6, 5, 3, 4, 1, 2, 0);
        else           v = bitswap8(v, 7, 6, 5, 1, 2, 4, 3, 0);
    } else if (BIT(p, 0)) {
        v = bitswap8(v, 7, 6, 5, 2, 1, 3, 4, 0);
    }
    return v;
}

static uint8_t type2b(uint8_t v, uint8_t p, unsigned s) {
    if (s == 0) v = bitswap8(v, 1, 3, 4, 6, 5, 7, 0, 2);
    if (s == 1) v = bitswap8(v, 0, 1, 5, 4, 7, 3, 2, 6);
    if (s == 2) v = bitswap8(v, 3, 5, 4, 1, 6, 2, 0, 7);
    if (s == 3) v = bitswap8(v, 5, 2, 3, 0, 4, 7, 6, 1);
    if (BIT(v, 7) && BIT(v, 3)) v ^= (1 << 6) | (1 << 4) | (1 << 0);
    if (BIT(v, 7)) v ^= (1 << 2);
    if (BIT(v, 5)) v ^= (1 << 7) | (1 << 3);
    if (BIT(v, 1)) v ^= (1 << 5);
    if (BIT(v, 4)) v ^= (1 << 7) | (1 << 5) | (1 << 3) | (1 << 1);
    if (BIT(v, 7) && BIT(v, 5)) v ^= (1 << 4) | (1 << 0);
    if (BIT(v, 5) && BIT(v, 1)) v ^= (1 << 4) | (1 << 0);
    if (BIT(v, 6)) v ^= (1 << 7) | (1 << 5);
    if (BIT(v, 3)) v ^= (1 << 7) | (1 << 6) | (1 << 5) | (1 << 1);
    if (BIT(v, 2)) v ^= (1 << 3) | (1 << 1);
    v ^= (1 << 7) | (1 << 3) | (1 << 2) | (1 << 1);
    if (BIT(p, 3)) v ^= (1 << 6) | (1 << 3) | (1 << 1);
    if (BIT(p, 2)) v ^= (1 << 7) | (1 << 6) | (1 << 5) | (1 << 3) | (1 << 2) | (1 << 1);
    if (BIT(p, 1)) v ^= (1 << 7);
    if (BIT(p, 0)) v ^= (1 << 5) | (1 << 2);
    return v;
}

static uint8_t type3a(uint8_t v, uint8_t p, unsigned s) {
    if (s == 0) v = bitswap8(v, 5, 3, 1, 7, 0, 2, 6, 4);
    if (s == 1) v = bitswap8(v, 3, 1, 2, 5, 4, 7, 0, 6);
    if (s == 2) v = bitswap8(v, 5, 6, 1, 2, 7, 0, 4, 3);
    if (s == 3) v = bitswap8(v, 5, 6, 7, 0, 4, 2, 1, 3);
    if (BIT(v, 2)) v ^= (1 << 7) | (1 << 5) | (1 << 4);
    if (BIT(v, 3)) v ^= (1 << 0);
    if (BIT(p, 0)) v = bitswap8(v, 7, 2, 5, 4, 3, 1, 0, 6);
    if (BIT(v, 1)) v ^= (1 << 6) | (1 << 0);
    if (BIT(v, 3)) v ^= (1 << 4) | (1 << 2) | (1 << 1);
    if (BIT(p, 3)) v ^= (1 << 4) | (1 << 3);
    if (BIT(v, 3)) v = bitswap8(v, 5, 6, 7, 4, 3, 2, 1, 0);
    if (BIT(v, 5)) v ^= (1 << 2) | (1 << 1);
    v ^= (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3);
    if (BIT(p, 2)) v ^= (1 << 7);
    if (BIT(p, 1)) v ^= (1 << 4);
    if (BIT(p, 0)) v ^= (1 << 0);
    return v;
}

static uint8_t type3b(uint8_t v, uint8_t p, unsigned s) {
    if (s == 0) v = bitswap8(v, 3, 7, 5, 4, 0, 6, 2, 1);
    if (s == 1) v = bitswap8(v, 7, 5, 4, 6, 1, 2, 0, 3);
    if (s == 2) v = bitswap8(v, 7, 4, 3, 0, 5, 1, 6, 2);
    if (s == 3) v = bitswap8(v, 2, 6, 4, 1, 3, 7, 0, 5);
    if (BIT(v, 2)) v ^= (1 << 7);
    if (BIT(v, 7)) v = bitswap8(v, 7, 6, 3, 4, 5, 2, 1, 0);
    if (BIT(p, 3)) v ^= (1 << 7);
    if (BIT(v, 4)) v ^= (1 << 6);
    if (BIT(v, 1)) v ^= (1 << 6) | (1 << 4) | (1 << 2);
    if (BIT(v, 7) && BIT(v, 6)) v ^= (1 << 1);
    if (BIT(v, 7)) v ^= (1 << 1);
    if (BIT(p, 3)) v ^= (1 << 7);
    if (BIT(p, 2)) v ^= (1 << 0);
    if (BIT(p, 3)) v = bitswap8(v, 4, 6, 3, 2, 5, 0, 1, 7);
    if (BIT(v, 4)) v ^= (1 << 1);
    if (BIT(v, 5)) v ^= (1 << 4);
    if (BIT(v, 7)) v ^= (1 << 2);
    v ^= (1 << 5) | (1 << 3) | (1 << 2);
    if (BIT(p, 1)) v ^= (1 << 7);
    if (BIT(p, 0)) v ^= (1 << 3);
    return v;
}

static uint8_t decrypt_internal(uint8_t val, uint8_t key, int opcode) {
    key ^= 0xff;
    if (key == 0x00) return val;  // no encryption

    unsigned type = 0;
    type ^= BIT(key, 0) << 0;
    type ^= BIT(key, 2) << 0;
    type ^= BIT(key, 0) << 1;
    type ^= BIT(key, 1) << 1;
    type ^= BIT(key, 2) << 1;
    type ^= BIT(key, 4) << 1;
    type ^= BIT(key, 4) << 2;
    type ^= BIT(key, 5) << 2;

    unsigned swap = 0;
    swap ^= BIT(key, 0) << 0;
    swap ^= BIT(key, 1) << 0;
    swap ^= BIT(key, 2) << 1;
    swap ^= BIT(key, 3) << 1;

    uint8_t param = 0;
    param ^= BIT(key, 0) << 0;
    param ^= BIT(key, 0) << 1;
    param ^= BIT(key, 2) << 1;
    param ^= BIT(key, 3) << 1;
    param ^= BIT(key, 0) << 2;
    param ^= BIT(key, 1) << 2;
    param ^= BIT(key, 6) << 2;
    param ^= BIT(key, 1) << 3;
    param ^= BIT(key, 6) << 3;
    param ^= BIT(key, 7) << 3;

    if (!opcode) { param ^= 1; type ^= 1; }

    switch (type & 7) {
        default:
        case 0: return type0(val, param, swap);
        case 1: return type0(val, param, swap);
        case 2: return type1a(val, param, swap);
        case 3: return type1b(val, param, swap);
        case 4: return type2a(val, param, swap);
        case 5: return type2b(val, param, swap);
        case 6: return type3a(val, param, swap);
        case 7: return type3b(val, param, swap);
    }
}

static uint8_t decrypt_one(const uint8_t *key, unsigned addr, uint8_t val, int opcode) {
    // table index from address bits 0xFD57
    static const int bits[12] = {15, 14, 13, 12, 11, 10, 8, 6, 4, 2, 1, 0};
    unsigned tbl = 0;
    for (int i = 0; i < 12; i++)
        tbl |= ((addr >> bits[i]) & 1) << (11 - i);
    return decrypt_internal(val, key[tbl | (opcode ? 0x0000 : 0x1000)], opcode);
}

void mc8123_decode(const uint8_t *rom, const uint8_t *key,
                   uint8_t *opcodes, uint8_t *data, unsigned length) {
    for (unsigned i = 0; i < length; i++) {
        unsigned adr = (i >= 0xc000) ? ((i & 0x3fff) | 0x8000) : i;
        uint8_t src = rom[i];
        opcodes[i] = decrypt_one(key, adr, src, 1);
        data[i]    = decrypt_one(key, adr, src, 0);
    }
}

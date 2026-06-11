"""NEC MC-8123 decryption — faithful port of MAME src/devices/cpu/z80/mc8123.cpp
(algorithm by Nicola Salmoria, tables by David Widel; BSD-3-Clause).

Wonder Boy: Monster Land (wbml) main CPU = MC-8123 (317-0043).
The key file (317-0043.key, 8KB) selects, per 12 address bits and
opcode/data fetch, one of several bit-mangling functions.

Usage:
    python tools/mc8123.py
Reads  roms/wbml/{epr-11031a.90, epr-11032.91, epr-11033.92, 317-0043.key}
Writes build/main_opcodes.bin and build/main_data.bin (0x20000 each,
region layout identical to MAME: 0x0000 fixed ROM, 0x10000+ banked ROMs,
hole 0x8000-0xFFFF filled with 0xFF).
"""

import sys
from pathlib import Path


def BIT(v, n):
    return (v >> n) & 1


def BITS(*bits):
    r = 0
    for b in bits:
        r |= 1 << b
    return r


def bitswap8(v, *src):
    """bitswap<8>(val, b7..b0): result bit (7-i) = val bit src[i]."""
    r = 0
    for i, b in enumerate(src):
        r |= ((v >> b) & 1) << (7 - i)
    return r


def decrypt_type0(val, param, swap):
    if swap == 0: val = bitswap8(val, 7, 5, 3, 1, 2, 0, 6, 4)
    if swap == 1: val = bitswap8(val, 5, 3, 7, 2, 1, 0, 4, 6)
    if swap == 2: val = bitswap8(val, 0, 3, 4, 6, 7, 1, 5, 2)
    if swap == 3: val = bitswap8(val, 0, 7, 3, 2, 6, 4, 1, 5)

    if BIT(param, 3) and BIT(val, 7):
        val ^= BITS(5, 3, 0)
    if BIT(param, 2) and BIT(val, 6):
        val ^= BITS(7, 2, 1)
    if BIT(val, 6): val ^= BITS(7)
    if BIT(param, 1) and BIT(val, 7):
        val ^= BITS(6)
    if BIT(val, 2): val ^= BITS(5, 0)
    val ^= BITS(4, 3, 1)
    if BIT(param, 2): val ^= BITS(5, 2, 0)
    if BIT(param, 1): val ^= BITS(7, 6)
    if BIT(param, 0): val ^= BITS(5, 0)
    if BIT(param, 0): val = bitswap8(val, 7, 6, 5, 1, 4, 3, 2, 0)
    return val


def decrypt_type1a(val, param, swap):
    if swap == 0: val = bitswap8(val, 4, 2, 6, 5, 3, 7, 1, 0)
    if swap == 1: val = bitswap8(val, 6, 0, 5, 4, 3, 2, 1, 7)
    if swap == 2: val = bitswap8(val, 2, 3, 6, 1, 4, 0, 7, 5)
    if swap == 3: val = bitswap8(val, 6, 5, 1, 3, 2, 7, 0, 4)

    if BIT(param, 2): val = bitswap8(val, 7, 6, 1, 5, 3, 2, 4, 0)

    if BIT(val, 1): val ^= BITS(0)
    if BIT(val, 6): val ^= BITS(3)
    if BIT(val, 7): val ^= BITS(6, 3)
    if BIT(val, 2): val ^= BITS(6, 3, 1)
    if BIT(val, 4): val ^= BITS(7, 6, 2)
    if BIT(val, 7) ^ BIT(val, 2):
        val ^= BITS(4)
    val ^= BITS(6, 3, 1, 0)
    if BIT(param, 3): val ^= BITS(7, 2)
    if BIT(param, 1): val ^= BITS(6, 3)
    if BIT(param, 0): val = bitswap8(val, 7, 6, 1, 4, 3, 2, 5, 0)
    return val


def decrypt_type1b(val, param, swap):
    if swap == 0: val = bitswap8(val, 1, 0, 3, 2, 5, 6, 4, 7)
    if swap == 1: val = bitswap8(val, 2, 0, 5, 1, 7, 4, 6, 3)
    if swap == 2: val = bitswap8(val, 6, 4, 7, 2, 0, 5, 1, 3)
    if swap == 3: val = bitswap8(val, 7, 1, 3, 6, 0, 2, 5, 4)

    if BIT(val, 2) and BIT(val, 0):
        val ^= BITS(7, 4)
    if BIT(val, 7): val ^= BITS(2)
    if BIT(val, 5): val ^= BITS(7, 2)
    if BIT(val, 1): val ^= BITS(5)
    if BIT(val, 6): val ^= BITS(1)
    if BIT(val, 4): val ^= BITS(6, 5)
    if BIT(val, 0): val ^= BITS(6, 2, 1)
    if BIT(val, 3): val ^= BITS(7, 6, 2, 1, 0)
    val ^= BITS(6, 4, 0)
    if BIT(param, 3): val ^= BITS(4, 1)
    if BIT(param, 2): val ^= BITS(7, 6, 3, 0)
    if BIT(param, 1): val ^= BITS(4, 3)
    if BIT(param, 0): val ^= BITS(6, 2, 1, 0)
    return val


def decrypt_type2a(val, param, swap):
    if swap == 0: val = bitswap8(val, 0, 1, 4, 3, 5, 6, 2, 7)
    if swap == 1: val = bitswap8(val, 6, 3, 0, 5, 7, 4, 1, 2)
    if swap == 2: val = bitswap8(val, 1, 6, 4, 5, 0, 3, 7, 2)
    if swap == 3: val = bitswap8(val, 4, 6, 7, 5, 2, 3, 1, 0)

    if BIT(val, 3) or (BIT(param, 1) and BIT(val, 2)):
        val = bitswap8(val, 6, 0, 7, 4, 3, 2, 1, 5)

    if BIT(val, 5): val ^= BITS(7)
    if BIT(val, 6): val ^= BITS(5)
    if BIT(val, 0): val ^= BITS(6)
    if BIT(val, 4): val ^= BITS(3, 0)
    if BIT(val, 1): val ^= BITS(2)
    val ^= BITS(7, 6, 5, 4, 1)
    if BIT(param, 2): val ^= BITS(4, 3, 2, 1, 0)

    if BIT(param, 3):
        if BIT(param, 0):
            val = bitswap8(val, 7, 6, 5, 3, 4, 1, 2, 0)
        else:
            val = bitswap8(val, 7, 6, 5, 1, 2, 4, 3, 0)
    elif BIT(param, 0):
        val = bitswap8(val, 7, 6, 5, 2, 1, 3, 4, 0)
    return val


def decrypt_type2b(val, param, swap):
    if swap == 0: val = bitswap8(val, 1, 3, 4, 6, 5, 7, 0, 2)
    if swap == 1: val = bitswap8(val, 0, 1, 5, 4, 7, 3, 2, 6)
    if swap == 2: val = bitswap8(val, 3, 5, 4, 1, 6, 2, 0, 7)
    if swap == 3: val = bitswap8(val, 5, 2, 3, 0, 4, 7, 6, 1)

    if BIT(val, 7) and BIT(val, 3):
        val ^= BITS(6, 4, 0)
    if BIT(val, 7): val ^= BITS(2)
    if BIT(val, 5): val ^= BITS(7, 3)
    if BIT(val, 1): val ^= BITS(5)
    if BIT(val, 4): val ^= BITS(7, 5, 3, 1)
    if BIT(val, 7) and BIT(val, 5):
        val ^= BITS(4, 0)
    if BIT(val, 5) and BIT(val, 1):
        val ^= BITS(4, 0)
    if BIT(val, 6): val ^= BITS(7, 5)
    if BIT(val, 3): val ^= BITS(7, 6, 5, 1)
    if BIT(val, 2): val ^= BITS(3, 1)
    val ^= BITS(7, 3, 2, 1)
    if BIT(param, 3): val ^= BITS(6, 3, 1)
    if BIT(param, 2): val ^= BITS(7, 6, 5, 3, 2, 1)
    if BIT(param, 1): val ^= BITS(7)
    if BIT(param, 0): val ^= BITS(5, 2)
    return val


def decrypt_type3a(val, param, swap):
    if swap == 0: val = bitswap8(val, 5, 3, 1, 7, 0, 2, 6, 4)
    if swap == 1: val = bitswap8(val, 3, 1, 2, 5, 4, 7, 0, 6)
    if swap == 2: val = bitswap8(val, 5, 6, 1, 2, 7, 0, 4, 3)
    if swap == 3: val = bitswap8(val, 5, 6, 7, 0, 4, 2, 1, 3)

    if BIT(val, 2): val ^= BITS(7, 5, 4)
    if BIT(val, 3): val ^= BITS(0)
    if BIT(param, 0): val = bitswap8(val, 7, 2, 5, 4, 3, 1, 0, 6)
    if BIT(val, 1): val ^= BITS(6, 0)
    if BIT(val, 3): val ^= BITS(4, 2, 1)
    if BIT(param, 3): val ^= BITS(4, 3)
    if BIT(val, 3): val = bitswap8(val, 5, 6, 7, 4, 3, 2, 1, 0)
    if BIT(val, 5): val ^= BITS(2, 1)
    val ^= BITS(6, 5, 4, 3)
    if BIT(param, 2): val ^= BITS(7)
    if BIT(param, 1): val ^= BITS(4)
    if BIT(param, 0): val ^= BITS(0)
    return val


def decrypt_type3b(val, param, swap):
    if swap == 0: val = bitswap8(val, 3, 7, 5, 4, 0, 6, 2, 1)
    if swap == 1: val = bitswap8(val, 7, 5, 4, 6, 1, 2, 0, 3)
    if swap == 2: val = bitswap8(val, 7, 4, 3, 0, 5, 1, 6, 2)
    if swap == 3: val = bitswap8(val, 2, 6, 4, 1, 3, 7, 0, 5)

    if BIT(val, 2): val ^= BITS(7)
    if BIT(val, 7): val = bitswap8(val, 7, 6, 3, 4, 5, 2, 1, 0)
    if BIT(param, 3): val ^= BITS(7)
    if BIT(val, 4): val ^= BITS(6)
    if BIT(val, 1): val ^= BITS(6, 4, 2)
    if BIT(val, 7) and BIT(val, 6):
        val ^= BITS(1)
    if BIT(val, 7): val ^= BITS(1)
    if BIT(param, 3): val ^= BITS(7)
    if BIT(param, 2): val ^= BITS(0)
    if BIT(param, 3): val = bitswap8(val, 4, 6, 3, 2, 5, 0, 1, 7)
    if BIT(val, 4): val ^= BITS(1)
    if BIT(val, 5): val ^= BITS(4)
    if BIT(val, 7): val ^= BITS(2)
    val ^= BITS(5, 3, 2)
    if BIT(param, 1): val ^= BITS(7)
    if BIT(param, 0): val ^= BITS(3)
    return val


_TYPE_FUNCS = (decrypt_type0, decrypt_type0,
               decrypt_type1a, decrypt_type1b,
               decrypt_type2a, decrypt_type2b,
               decrypt_type3a, decrypt_type3b)


def decrypt_internal(val, key, opcode):
    key ^= 0xff
    if key == 0x00:  # no encryption (RAM areas etc.)
        return val

    typ = 0
    typ ^= BIT(key, 0) << 0
    typ ^= BIT(key, 2) << 0
    typ ^= BIT(key, 0) << 1
    typ ^= BIT(key, 1) << 1
    typ ^= BIT(key, 2) << 1
    typ ^= BIT(key, 4) << 1
    typ ^= BIT(key, 4) << 2
    typ ^= BIT(key, 5) << 2

    swap = 0
    swap ^= BIT(key, 0) << 0
    swap ^= BIT(key, 1) << 0
    swap ^= BIT(key, 2) << 1
    swap ^= BIT(key, 3) << 1

    param = 0
    param ^= BIT(key, 0) << 0
    param ^= BIT(key, 0) << 1
    param ^= BIT(key, 2) << 1
    param ^= BIT(key, 3) << 1
    param ^= BIT(key, 0) << 2
    param ^= BIT(key, 1) << 2
    param ^= BIT(key, 6) << 2
    param ^= BIT(key, 1) << 3
    param ^= BIT(key, 6) << 3
    param ^= BIT(key, 7) << 3

    if not opcode:
        param ^= 1
        typ ^= 1

    return _TYPE_FUNCS[typ & 7](val, param, swap)


def decrypt(key_rom, addr, val, opcode):
    # translation table index comes from address bits 0xFD57
    tbl = 0
    for i, b in enumerate((15, 14, 13, 12, 11, 10, 8, 6, 4, 2, 1, 0)):
        tbl |= ((addr >> b) & 1) << (11 - i)
    return decrypt_internal(val, key_rom[tbl | (0x0000 if opcode else 0x1000)], opcode)


def decode_region(region, key_rom, ranges):
    """Return (opcodes, data) views of `region`. `ranges` lists (start, end)
    spans that actually contain ROM; the rest is left untouched.
    Region offsets >= 0xC000 decrypt as if mapped at 0x8000-0xBFFF (banked)."""
    opcodes = bytearray(region)
    data = bytearray(region)
    for start, end in ranges:
        for i in range(start, end):
            adr = ((i & 0x3fff) | 0x8000) if i >= 0xc000 else i
            src = region[i]
            opcodes[i] = decrypt(key_rom, adr, src, True)
            data[i] = decrypt(key_rom, adr, src, False)
    return bytes(opcodes), bytes(data)


def main():
    root = Path(__file__).resolve().parent.parent
    romdir = root / "roms" / "wbml"
    outdir = root / "build"
    outdir.mkdir(exist_ok=True)

    key_rom = (romdir / "317-0043.key").read_bytes()
    assert len(key_rom) == 0x2000, "key must be 8KB"

    region = bytearray(b"\xff" * 0x20000)
    region[0x00000:0x08000] = (romdir / "epr-11031a.90").read_bytes()
    region[0x10000:0x18000] = (romdir / "epr-11032.91").read_bytes()
    region[0x18000:0x20000] = (romdir / "epr-11033.92").read_bytes()

    opcodes, data = decode_region(
        bytes(region), key_rom, [(0x00000, 0x08000), (0x10000, 0x20000)])

    (outdir / "main_opcodes.bin").write_bytes(opcodes)
    (outdir / "main_data.bin").write_bytes(data)
    print("wrote build/main_opcodes.bin and build/main_data.bin (128KB each)")
    print("reset vector (opcode view):",
          " ".join(f"{b:02X}" for b in opcodes[0:16]))
    print("0x0038 IRQ   (opcode view):",
          " ".join(f"{b:02X}" for b in opcodes[0x38:0x48]))
    print("0x0066 NMI   (opcode view):",
          " ".join(f"{b:02X}" for b in opcodes[0x66:0x6e]))


if __name__ == "__main__":
    sys.exit(main())

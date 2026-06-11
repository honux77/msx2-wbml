"""Minimal Z80 disassembler for the MC-8123-decoded wbml ROM.

Opcodes come from build/main_opcodes.bin (M1 view), operands from
build/main_data.bin (data view) -- matching how the real CPU fetches.
Covers the common opcode set; unknown opcodes print as DB.

Usage: python tools/disasm.py <start_hex> <count>
"""
import sys

op = open("build/main_opcodes.bin", "rb").read()
da = open("build/main_data.bin", "rb").read()

R = ["b", "c", "d", "e", "h", "l", "(hl)", "a"]
RP = ["bc", "de", "hl", "sp"]
CC = ["nz", "z", "nc", "c", "po", "pe", "p", "m"]
ALU = ["add a,", "adc a,", "sub ", "sbc a,", "and ", "xor ", "or ", "cp "]


def d8(a):
    return da[a]


def d16(a):
    return da[a] | (da[a + 1] << 8)


def s8(a):
    v = da[a]
    return v - 256 if v >= 128 else v


def disas(addr):
    """Return (text, length)."""
    o = op[addr]
    if o == 0xCB:
        o2 = op[addr + 1]
        reg = R[o2 & 7]
        if o2 < 0x40:
            ops = ["rlc", "rrc", "rl", "rr", "sla", "sra", "sll", "srl"]
            return f"{ops[o2 >> 3]} {reg}", 2
        bit = (o2 >> 3) & 7
        pfx = ["bit", "res", "set"][(o2 >> 6) - 1]
        return f"{pfx} {bit},{reg}", 2
    if o == 0xED:
        o2 = op[addr + 1]
        m = {0x44: "neg", 0x45: "retn", 0x4D: "reti", 0x46: "im 0",
             0x56: "im 1", 0x5E: "im 2", 0xB0: "ldir", 0xB8: "lddr",
             0xB1: "cpir", 0xB9: "cpdr", 0xA0: "ldi", 0xA8: "ldd",
             0x47: "ld i,a", 0x4F: "ld r,a", 0x57: "ld a,i", 0x5F: "ld a,r",
             0x67: "rrd", 0x6F: "rld"}
        if o2 in m:
            return m[o2], 2
        if (o2 & 0xCF) == 0x4B:  # ld rp,(nn)
            return f"ld {RP[(o2>>4)&3]},({d16(addr+2):04X})", 4
        if (o2 & 0xCF) == 0x43:  # ld (nn),rp
            return f"ld ({d16(addr+2):04X}),{RP[(o2>>4)&3]}", 4
        if (o2 & 0xC7) == 0x42:  # sbc/adc hl,rp
            mn = "sbc" if not (o2 & 8) else "adc"
            return f"{mn} hl,{RP[(o2>>4)&3]}", 2
        if (o2 & 0xC7) == 0x40:  # in r,(c)
            return f"in {R[(o2>>3)&7]},(c)", 2
        if (o2 & 0xC7) == 0x41:  # out (c),r
            return f"out (c),{R[(o2>>3)&7]}", 2
        return f"db edh,{o2:02X}h", 2
    if o in (0xDD, 0xFD):
        ix = "ix" if o == 0xDD else "iy"
        return f"[{ix} prefix]", 1  # not fully decoded
    # main table
    if o == 0x00: return "nop", 1
    if o == 0x76: return "halt", 1
    if o == 0xF3: return "di", 1
    if o == 0xFB: return "ei", 1
    if o == 0x07: return "rlca", 1
    if o == 0x0F: return "rrca", 1
    if o == 0x17: return "rla", 1
    if o == 0x1F: return "rra", 1
    if o == 0x2F: return "cpl", 1
    if o == 0x37: return "scf", 1
    if o == 0x3F: return "ccf", 1
    if o == 0x27: return "daa", 1
    if o == 0x08: return "ex af,af'", 1
    if o == 0xD9: return "exx", 1
    if o == 0xEB: return "ex de,hl", 1
    if o == 0xE3: return "ex (sp),hl", 1
    if o == 0xE9: return "jp (hl)", 1
    if o == 0xF9: return "ld sp,hl", 1
    if o == 0xC3: return f"jp {d16(addr+1):04X}", 3
    if o == 0xCD: return f"call {d16(addr+1):04X}", 3
    if o == 0xC9: return "ret", 1
    if o == 0x18: return f"jr {addr+2+s8(addr+1):04X}", 2
    if o == 0x10: return f"djnz {addr+2+s8(addr+1):04X}", 2
    if (o & 0xE7) == 0x20:  # jr cc
        return f"jr {CC[(o>>3)&3]},{addr+2+s8(addr+1):04X}", 2
    if (o & 0xC7) == 0xC2:  # jp cc
        return f"jp {CC[(o>>3)&7]},{d16(addr+1):04X}", 3
    if (o & 0xC7) == 0xC4:  # call cc
        return f"call {CC[(o>>3)&7]},{d16(addr+1):04X}", 3
    if (o & 0xC7) == 0xC0:  # ret cc
        return f"ret {CC[(o>>3)&7]}", 1
    if (o & 0xC7) == 0xC7:  # rst
        return f"rst {o & 0x38:02X}", 1
    if (o & 0xCF) == 0xC5:  # push
        return f"push {['bc','de','hl','af'][(o>>4)&3]}", 1
    if (o & 0xCF) == 0xC1:  # pop
        return f"pop {['bc','de','hl','af'][(o>>4)&3]}", 1
    if o == 0xDB: return f"in a,({d8(addr+1):02X})", 2
    if o == 0xD3: return f"out ({d8(addr+1):02X}),a", 2
    if (o & 0xCF) == 0x01:  # ld rp,nn
        return f"ld {RP[(o>>4)&3]},{d16(addr+1):04X}", 3
    if (o & 0xCF) == 0x09:  # add hl,rp
        return f"add hl,{RP[(o>>4)&3]}", 1
    if (o & 0xCF) == 0x03:  # inc rp
        return f"inc {RP[(o>>4)&3]}", 1
    if (o & 0xCF) == 0x0B:  # dec rp
        return f"dec {RP[(o>>4)&3]}", 1
    if (o & 0xC7) == 0x04:  # inc r
        return f"inc {R[(o>>3)&7]}", 1
    if (o & 0xC7) == 0x05:  # dec r
        return f"dec {R[(o>>3)&7]}", 1
    if (o & 0xC7) == 0x06:  # ld r,n
        return f"ld {R[(o>>3)&7]},{d8(addr+1):02X}", 2
    if o == 0x32: return f"ld ({d16(addr+1):04X}),a", 3
    if o == 0x3A: return f"ld a,({d16(addr+1):04X})", 3
    if o == 0x22: return f"ld ({d16(addr+1):04X}),hl", 3
    if o == 0x2A: return f"ld hl,({d16(addr+1):04X})", 3
    if o == 0x02: return "ld (bc),a", 1
    if o == 0x12: return "ld (de),a", 1
    if o == 0x0A: return "ld a,(bc)", 1
    if o == 0x1A: return "ld a,(de)", 1
    if o == 0x36: return f"ld (hl),{d8(addr+1):02X}", 2
    if 0x40 <= o <= 0x7F:  # ld r,r'
        return f"ld {R[(o>>3)&7]},{R[o&7]}", 1
    if 0x80 <= o <= 0xBF:  # alu r
        return f"{ALU[(o>>3)&7]}{R[o&7]}", 1
    if (o & 0xC7) == 0xC6:  # alu n
        return f"{ALU[(o>>3)&7]}{d8(addr+1):02X}", 2
    return f"db {o:02X}h", 1


def main():
    start = int(sys.argv[1], 16)
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 32
    a = start
    for _ in range(count):
        txt, ln = disas(a)
        raw = " ".join(f"{op[a+i]:02X}" for i in range(ln))
        print(f"{a:04X}: {raw:<12} {txt}")
        a += ln


if __name__ == "__main__":
    main()

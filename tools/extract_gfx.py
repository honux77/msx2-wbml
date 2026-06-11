"""Extract wbml graphics for inspection and (later) MSX conversion.

Outputs into build/:
  tiles.png          4096 8x8 3bpp tiles, 64x64 grid (gray ramp, no palette)
  sprites_raw.png    raw 4bpp nibble view of the 4 sprite ROM banks
                     (pen 0 = black/transparent, pen 15 = red end-of-row marker)
  palette_proms.png  the 256-color gamut from the three palette PROMs
                     (palette RAM values index into this table at runtime)

Tile format  : 3 bitplanes, one per ROM (epr-11034.4 = MSB), 8 bytes/tile/plane.
Sprite format: free-form nibble stream; rows are read from spriteram-provided
               addresses, pen 15 terminates a row (see MAME system1_v.cpp).
"""

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ROMDIR = ROOT / "roms" / "wbml"
OUT = ROOT / "build"


def extract_tiles():
    planes = [
        (ROMDIR / "epr-11034.4").read_bytes(),   # plane 0 = MSB (bit 2)
        (ROMDIR / "epr-11035.5").read_bytes(),   # bit 1
        (ROMDIR / "epr-11036.6").read_bytes(),   # bit 0
    ]
    ntiles = len(planes[0]) // 8  # 4096
    cols = 64
    rows = (ntiles + cols - 1) // cols
    img = Image.new("P", (cols * 8, rows * 8))
    img.putpalette(bytes(v for i in range(8) for v in (i * 36, i * 36, i * 36))
                   + bytes(248 * 3))
    px = img.load()
    for t in range(ntiles):
        tx, ty = (t % cols) * 8, (t // cols) * 8
        for y in range(8):
            b2 = planes[0][t * 8 + y]
            b1 = planes[1][t * 8 + y]
            b0 = planes[2][t * 8 + y]
            for x in range(8):
                bit = 7 - x
                px[tx + x, ty + y] = (((b2 >> bit) & 1) << 2 |
                                      ((b1 >> bit) & 1) << 1 |
                                      ((b0 >> bit) & 1))
    img.save(OUT / "tiles.png")
    print(f"tiles.png: {ntiles} tiles")


def extract_sprites_raw():
    # region order as in MAME wbml ROM_START (banks 0-3)
    names = ["epr-11028.87", "epr-11027.86", "epr-11030.89", "epr-11029.88"]
    data = b"".join((ROMDIR / n).read_bytes() for n in names)
    width = 256          # nibbles per row
    stride = width // 2  # bytes per row
    height = len(data) // stride
    img = Image.new("P", (width, height))
    pal = []
    for i in range(16):
        if i == 0:
            pal += [0, 0, 0]
        elif i == 15:
            pal += [255, 0, 0]
        else:
            v = 40 + i * 14
            pal += [v, v, v]
    img.putpalette(bytes(pal) + bytes(240 * 3))
    px = img.load()
    for i, byte in enumerate(data):
        y, xb = divmod(i, stride)
        px[xb * 2, y] = byte >> 4
        px[xb * 2 + 1, y] = byte & 0x0F
    img.save(OUT / "sprites_raw.png")
    print(f"sprites_raw.png: {len(data)} bytes, {height} rows")


def extract_palette_proms():
    r = (ROMDIR / "pr11026.20").read_bytes()
    g = (ROMDIR / "pr11025.14").read_bytes()
    b = (ROMDIR / "pr11024.8").read_bytes()

    def level(v):  # 4-bit DAC weights from MAME system1_palette()
        return (0x0e * (v & 1) + 0x1f * (v >> 1 & 1) +
                0x43 * (v >> 2 & 1) + 0x8f * (v >> 3 & 1))

    cell = 16
    img = Image.new("RGB", (16 * cell, 16 * cell))
    px = img.load()
    colors = set()
    for i in range(256):
        rgb = (level(r[i]), level(g[i]), level(b[i]))
        colors.add(rgb)
        cx, cy = (i % 16) * cell, (i // 16) * cell
        for y in range(cell):
            for x in range(cell):
                px[cx + x, cy + y] = rgb
    img.save(OUT / "palette_proms.png")
    print(f"palette_proms.png: {len(colors)} distinct colors in gamut")


if __name__ == "__main__":
    OUT.mkdir(exist_ok=True)
    extract_tiles()
    extract_sprites_raw()
    extract_palette_proms()

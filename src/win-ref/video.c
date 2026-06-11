// System 2 video renderer — C port of MAME system1_v.cpp (system2 path).
#include "video.h"
#include <string.h>

// 8 tilemap pages, each 256x256 px. Pixel value = (color << 3) | pen,
// pen 0 = transparent. Rebuilt every frame (no dirty tracking needed at 60fps).
static uint16_t page_pix[8][256 * 256];
static uint16_t sprite_bmp[256 * SCREEN_W];  // sprite line buffer for the frame
static uint8_t  colortab[256][3];            // indirect color -> RGB

static uint8_t prom_level(uint8_t v) {
    // 4-bit DAC weights from MAME system1_palette().
    return (uint8_t)(0x0e * ((v >> 0) & 1) + 0x1f * ((v >> 1) & 1) +
                     0x43 * ((v >> 2) & 1) + 0x8f * ((v >> 3) & 1));
}

static void build_colortab(system2 *m) {
    for (int i = 0; i < 256; i++) {
        colortab[i][0] = prom_level(m->color_prom[i + 0 * 256]);
        colortab[i][1] = prom_level(m->color_prom[i + 1 * 256]);
        colortab[i][2] = prom_level(m->color_prom[i + 2 * 256]);
    }
}

static void build_page(system2 *m, int page) {
    const uint8_t *vr = m->videoram + page * 0x800;
    uint16_t *dst = page_pix[page];
    for (int ty = 0; ty < 32; ty++) {
        for (int tx = 0; tx < 32; tx++) {
            int idx = ty * 32 + tx;
            uint32_t td = vr[idx * 2] | (vr[idx * 2 + 1] << 8);
            uint32_t code = ((td >> 4) & 0x800) | (td & 0x7ff);
            uint32_t color = (td >> 5) & 0xff;
            const uint8_t *p0 = m->tilerom + 0x00000 + code * 8;  // bit2 (MSB)
            const uint8_t *p1 = m->tilerom + 0x08000 + code * 8;  // bit1
            const uint8_t *p2 = m->tilerom + 0x10000 + code * 8;  // bit0
            for (int row = 0; row < 8; row++) {
                uint8_t b2 = p0[row], b1 = p1[row], b0 = p2[row];
                uint16_t *line = dst + (ty * 8 + row) * 256 + tx * 8;
                for (int x = 0; x < 8; x++) {
                    int bit = 7 - x;
                    uint8_t pen = (uint8_t)(((b2 >> bit) & 1) << 2 |
                                            ((b1 >> bit) & 1) << 1 |
                                            ((b0 >> bit) & 1));
                    line[x] = (uint16_t)((color << 3) | pen);
                }
            }
        }
    }
}

// Port of system1_state::draw_sprites (writes 9-bit values into sprite_bmp).
static void draw_sprites(system2 *m, int xoffset) {
    const uint32_t gfxbanks = SPRITEROM_SIZE / 0x8000;
    const int flip = m->flip;

    memset(sprite_bmp, 0, sizeof(sprite_bmp));
    memset(m->sprite_collide, 0, sizeof(m->sprite_collide));

    for (int spritenum = 0; spritenum < 32; spritenum++) {
        const uint8_t *sd = &m->spriteram[spritenum * 0x10];
        uint16_t srcaddr = sd[6] + (sd[7] << 8);
        const uint16_t stride = sd[4] + (sd[5] << 8);
        uint8_t bank = ((sd[3] & 0x80) >> 7) | ((sd[3] & 0x40) >> 5) | ((sd[3] & 0x20) >> 3);
        const int xstart = ((sd[2] | (sd[3] << 8)) & 0x1ff) + xoffset;
        int bottom = sd[1] + 1;
        int top = sd[0] + 1;
        const uint16_t palbase = spritenum * 0x10;

        if (sd[0] == 0xff)  // 0xff in first byte disables all sprites
            return;

        bank %= gfxbanks;
        const uint8_t *gfxbase = &m->spriterom[bank * 0x8000];

        if (flip) { int t = top; top = 256 - bottom; bottom = 256 - t; }

        for (int y = top; y < bottom; y++) {
            if (y < 0 || y >= 256) { srcaddr += stride; continue; }
            uint16_t *destbase = &sprite_bmp[y * SCREEN_W];
            srcaddr += stride;

            int addrdelta = (srcaddr & 0x8000) ? -1 : 1;
            for (int x = xstart, curaddr = srcaddr;; x += 4, curaddr += addrdelta) {
                uint8_t color1, color2;
                const uint8_t d = gfxbase[curaddr & 0x7fff];
                if (!(curaddr & 0x8000)) { color1 = d >> 4; color2 = d & 0x0f; }
                else                     { color1 = d & 0x0f; color2 = d >> 4; }

                if (color1 == 0x0f) break;
                if (color1 != 0) {
                    for (int i = 0; i < 2; i++) {
                        int effx = flip ? 0x1fe - (x + i) : (x + i);
                        if (effx >= 0 && effx < SCREEN_W) {
                            int prev = destbase[effx];
                            if ((prev & 0x0f) != 0)
                                m->sprite_collide[((prev >> 4) & 0x1f) + 32 * spritenum] =
                                    m->sprite_collide_summary = 1;
                            destbase[effx] = color1 | palbase;
                        }
                    }
                }
                if (color2 == 0x0f) break;
                if (color2 != 0) {
                    for (int i = 0; i < 2; i++) {
                        int effx = flip ? 0x1fe - (x + 2 + i) : (x + 2 + i);
                        if (effx >= 0 && effx < SCREEN_W) {
                            int prev = destbase[effx];
                            if ((prev & 0x0f) != 0)
                                m->sprite_collide[((prev >> 4) & 0x1f) + 32 * spritenum] =
                                    m->sprite_collide_summary = 1;
                            destbase[effx] = color2 | palbase;
                        }
                    }
                }
            }
        }
    }
}

void video_render(system2 *m, uint32_t *fb) {
    build_colortab(m);

    if (m->video_mode & 0x10) {  // screen blank
        memset(fb, 0, SCREEN_W * SCREEN_H * sizeof(uint32_t));
        return;
    }

    for (int p = 0; p < 8; p++)
        build_page(m, p);

    const uint8_t *vr = m->videoram;
    // 4 independent background pages (page0 reserved for fixed foreground)
    uint16_t *bg[4] = {
        page_pix[vr[0x740] & 7], page_pix[vr[0x742] & 7],
        page_pix[vr[0x744] & 7], page_pix[vr[0x746] & 7],
    };
    uint16_t *fg = page_pix[0];

    // wbml uses screen_update_system2 (a single screen-wide X scroll), NOT the
    // per-row rowscroll variant. Reading 0x7c0+y*2 per row would scatter the
    // background; the scroll comes from one register pair at 0x7c0/0x7c1.
    int rowscroll[32];
    int yscroll, sprxoffset, xscroll;
    if (!m->flip) {
        xscroll = ((vr[0x7c0] | (vr[0x7c1] << 8)) & 0x1ff) - 512 + 10;
        yscroll = vr[0x7ba];
        sprxoffset = 14;
    } else {
        xscroll = 512 + 512 + 10 - (((vr[0x7f6] | (vr[0x7f7] << 8)) & 0x1ff) - 512 + 10);
        yscroll = 512 + 512 - vr[0x784];
        sprxoffset = -14;
    }
    for (int y = 0; y < 32; y++)
        rowscroll[y] = xscroll;

    draw_sprites(m, sprxoffset);

    memset(m->mix_collide, 0, sizeof(m->mix_collide));

    for (int y = 0; y < SCREEN_H; y++) {
        const uint16_t *fgline = &fg[(y & 0xff) * 256];
        const uint16_t *sprline = &sprite_bmp[(y & 0xff) * SCREEN_W];
        uint32_t *dst = &fb[y * SCREEN_W];
        int bgy = (y + yscroll) & 0x1ff;
        int bgxscroll = rowscroll[(y >> 3) & 0x1f];
        const uint16_t *bgbase[2] = {
            &bg[(bgy >> 8) * 2 + 0][(bgy & 0xff) * 256],
            &bg[(bgy >> 8) * 2 + 1][(bgy & 0xff) * 256],
        };

        for (int x = 0; x < SCREEN_W; x++) {
            int bgx = ((x - bgxscroll) / 2) & 0x1ff;
            uint16_t fgpix = fgline[(x / 2) & 0xff];
            uint16_t bgpix = bgbase[bgx >> 8][bgx & 0xff];
            uint16_t sprpix = sprline[x];

            uint8_t lookup_index = (uint8_t)(
                (((sprpix & 0xf) == 0) << 0) |
                (((fgpix & 7) == 0) << 1) |
                (((fgpix >> 9) & 3) << 2) |
                (((bgpix & 7) == 0) << 4) |
                (((bgpix >> 9) & 3) << 5));
            uint8_t lv = m->lookup_prom[lookup_index];

            if (!(lv & 4))
                m->mix_collide[((lv & 8) << 2) | ((sprpix >> 4) & 0x1f)] =
                    m->mix_collide_summary = 1;

            lv &= 3;
            uint16_t palidx;
            if (lv == 0)      palidx = 0x000 | (sprpix & 0x1ff);
            else if (lv == 1) palidx = 0x200 | (fgpix & 0x1ff);
            else              palidx = 0x400 | (bgpix & 0x1ff);

            uint8_t ci = m->paletteram[palidx & 0x7ff];
            dst[x] = 0xff000000u | (colortab[ci][0] << 16) |
                     (colortab[ci][1] << 8) | colortab[ci][2];
        }
    }
}

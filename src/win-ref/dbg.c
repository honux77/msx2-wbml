// Headless diagnostic: run N frames, print CPU + video state. No SDL.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "machine.h"
#include "video.h"

int main(int argc, char **argv) {
    extern int g_io_log;
    g_io_log = 1;
    const char *romdir = (argc > 1) ? argv[1] : "roms/wbml";
    int frames = (argc > 2) ? atoi(argv[2]) : 240;
    static system2 m;
    if (machine_init(&m, romdir)) { printf("init failed\n"); return 1; }

    static uint32_t fb[SCREEN_W * SCREEN_H];
    for (int i = 0; i < frames; i++)
        machine_run_frame(&m, fb);

    // dump the final rendered frame for visual inspection
    FILE *pf = fopen("build/dbg.ppm", "wb");
    if (pf) {
        fprintf(pf, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
        for (int p = 0; p < SCREEN_W * SCREEN_H; p++) {
            uint32_t c = fb[p];
            fputc((c >> 16) & 0xff, pf); fputc((c >> 8) & 0xff, pf); fputc(c & 0xff, pf);
        }
        fclose(pf);
    }

    z80 *z = &m.maincpu;
    printf("PC=%04X SP=%04X AF=%02X.. BC=%04X DE=%04X HL=%04X cyc=%lu\n",
           z->pc, z->sp, z->a, (z->b << 8) | z->c, (z->d << 8) | z->e,
           (z->h << 8) | z->l, z->cyc);
    printf("video_mode=%02X flip=%d bank1=%d videoram_bank=%02X nmi_mask=%d\n",
           m.video_mode, m.flip, m.bank1, m.videoram_bank, m.sound_nmi_mask);
    printf("ppi_portc=%02X soundlatch=%02X\n", m.ppi_portc, m.soundlatch);

    int pal_nz = 0; for (int i = 0; i < 0x800; i++) if (m.paletteram[i]) pal_nz++;
    int vr_nz = 0;  for (int i = 0; i < VIDEORAM_SIZE; i++) if (m.videoram[i]) vr_nz++;
    int ram_nz = 0; for (int i = 0; i < 0x1000; i++) if (m.ram[i]) ram_nz++;
    int spr_nz = 0; for (int i = 0; i < 0x800; i++) if (m.spriteram[i]) spr_nz++;
    printf("nonzero: paletteram=%d videoram=%d ram=%d spriteram=%d\n",
           pal_nz, vr_nz, ram_nz, spr_nz);

    printf("videoram[0x740..0x747]:");
    for (int i = 0x740; i <= 0x747; i++) printf(" %02X", m.videoram[i]);
    printf("\n");

    // framebuffer stats
    int fb_nz = 0; for (int i = 0; i < SCREEN_W * SCREEN_H; i++) if ((fb[i] & 0xffffff)) fb_nz++;
    printf("framebuffer non-black: %d / %d\n", fb_nz, SCREEN_W * SCREEN_H);

    // single-step trace; annotate IN/OUT; fire periodic VBLANK IRQ so that
    // vblank-synced code progresses while tracing.
    int n = (argc > 3) ? atoi(argv[3]) : 48;
    int filter = (argc > 4) ? (int)strtol(argv[4], NULL, 16) : -1;  // only PC>=filter
    unsigned long next_irq = z->cyc + (4000000 / 60);
    g_io_log = 0;  // quiet during trace
    printf("--- trace (%d steps) ---\n", n);
    for (int i = 0; i < n; i++) {
        if (z->cyc >= next_irq) { z80_gen_int(z, 0xff); next_irq += 4000000 / 60; }
        uint16_t pc = z->pc;
        unsigned off = pc < 0x8000 ? pc : 0x10000 + m.bank1 * 0x4000 + (pc - 0x8000);
        uint8_t o0 = m.opcodes[off];
        uint8_t o1 = m.data[(off + 1) & (MAINROM_SIZE - 1)];
        if (filter < 0 || pc >= filter) {
            char note[32] = "";
            if (o0 == 0xDB) snprintf(note, sizeof note, " IN A,(%02X)", o1);
            else if (o0 == 0xD3) snprintf(note, sizeof note, " OUT (%02X),A", o1);
            printf("PC=%04X op=%02X A=%02X BC=%04X DE=%04X HL=%04X SP=%04X%s\n",
                   pc, o0, z->a, (z->b<<8)|z->c, (z->d<<8)|z->e, (z->h<<8)|z->l, z->sp, note);
        }
        z80_step(z);
    }
    return 0;
}

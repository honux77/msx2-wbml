// Headless diagnostic: run N frames, print CPU + video state. No SDL.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "machine.h"
#include "video.h"

int main(int argc, char **argv) {
    extern int g_io_log, g_sn_solo, g_sn_log; extern FILE *g_sn_logfile;
    g_io_log = 1;
    { char *e = getenv("SN_SOLO"); if (e) g_sn_solo = atoi(e); }
    { char *e = getenv("SN_LOG");  if (e) { g_sn_log = atoi(e); g_sn_logfile = stderr; } }
    const char *romdir = (argc > 1) ? argv[1] : "roms/wbml";
    int frames = (argc > 2) ? atoi(argv[2]) : 240;
    static system2 m;
    if (machine_init(&m, romdir)) { printf("init failed\n"); return 1; }

    static uint32_t fb[SCREEN_W * SCREEN_H];
    static int16_t audio[AUDIO_MAX_FRAME];
    // capture audio to a 16-bit mono WAV for inspection
    FILE *wf = fopen("build/dbg.wav", "wb");
    long total = 0;
    if (wf) for (int i = 0; i < 44; i++) fputc(0, wf);  // header placeholder
    // Optional 4th arg "play": auto-insert a coin and press start so the trace
    // reaches actual gameplay (where BGM/SFX really run).
    int play = (argc > 4 && strcmp(argv[4], "play") == 0);
    for (int i = 0; i < frames; i++) {
        if (play) {
            m.in.system = 0;
            if (i >= 30 && i < 40)  m.in.system = 0x01;  // coin1
            if (i >= 80 && i < 90)  m.in.system = 0x10;  // start1
        }
        int ns = machine_run_frame(&m, fb, audio);
        if (wf) { fwrite(audio, sizeof(int16_t), ns, wf); total += ns; }
    }
    if (wf) {
        // backfill canonical 44.1kHz mono PCM WAV header
        long datalen = total * 2, rifflen = 36 + datalen;
        fseek(wf, 0, SEEK_SET);
        fwrite("RIFF", 1, 4, wf); fputc(rifflen & 0xff, wf); fputc((rifflen>>8)&0xff, wf);
        fputc((rifflen>>16)&0xff, wf); fputc((rifflen>>24)&0xff, wf);
        fwrite("WAVEfmt ", 1, 8, wf);
        int32_t fmtlen = 16; int16_t pcm = 1, ch = 1, bps = 16; int32_t sr = AUDIO_SAMPLE_RATE;
        int32_t byterate = sr * 2; int16_t blockalign = 2;
        fwrite(&fmtlen,4,1,wf); fwrite(&pcm,2,1,wf); fwrite(&ch,2,1,wf);
        fwrite(&sr,4,1,wf); fwrite(&byterate,4,1,wf); fwrite(&blockalign,2,1,wf); fwrite(&bps,2,1,wf);
        fwrite("data",1,4,wf); fputc(datalen&0xff,wf); fputc((datalen>>8)&0xff,wf);
        fputc((datalen>>16)&0xff,wf); fputc((datalen>>24)&0xff,wf);
        fclose(wf);
    }

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
    {
        extern unsigned long g_sn_writes, g_latch_reads, g_snd_nmi, g_snd_irq,
                             g_snd_irq_taken, g_snd_loop, g_snd_driver;
        z80 *sc = &m.soundcpu;
        printf("[sound] PC=%04X iff1=%d halted=%d cyc=%lu\n",
               sc->pc, sc->iff1, sc->halted, sc->cyc);
        printf("[sound] sn_writes=%lu latch_reads=%lu nmi=%lu irq_gen=%lu irq_taken=%lu (over %d frames)\n",
               g_sn_writes, g_latch_reads, g_snd_nmi, g_snd_irq, g_snd_irq_taken, frames);
        printf("[sound] mainloop_turns=%lu music_driver_calls=%lu (expect ~%d at 4/frame)\n",
               g_snd_loop, g_snd_driver, frames * 4);
    }

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

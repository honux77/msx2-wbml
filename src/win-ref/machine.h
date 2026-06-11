// wbml (Sega System 2) machine: memory map, banking, PPI, two Z80s.
#ifndef MACHINE_H_
#define MACHINE_H_

#include <stdint.h>
#include "z80/z80.h"
#include "sn76489.h"

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_MAX_FRAME   1024   // upper bound on samples produced per frame

#define MAINROM_SIZE   0x20000
#define VIDEORAM_SIZE  0x4000   // 8 pages * 0x800
#define TILEROM_SIZE   0x18000  // 3 planes * 0x8000
#define SPRITEROM_SIZE 0x20000
#define SOUNDROM_SIZE  0x10000

// Input bits are active-low on the hardware; we store active-high here and
// invert on read. One byte each for P1, P2, SYSTEM, plus two DIP banks.
typedef struct {
    uint8_t p1, p2, system;   // active-high (1 = pressed)
    uint8_t swa, swb;         // DIP switches, raw (active-low semantics)
} sys_inputs;

typedef struct {
    z80 maincpu;
    z80 soundcpu;

    // Main CPU ROM, decoded into separate opcode/data views by MC-8123.
    uint8_t mainrom[MAINROM_SIZE];   // raw (unused after decode, kept for ref)
    uint8_t opcodes[MAINROM_SIZE];   // M1 fetch view
    uint8_t data[MAINROM_SIZE];      // data read view
    uint8_t key[0x2000];

    uint8_t ram[0x1000];        // c000-cfff
    uint8_t spriteram[0x800];   // d000-d7ff
    uint8_t paletteram[0x800];  // d800-dfff
    uint8_t videoram[VIDEORAM_SIZE];

    uint8_t mix_collide[64];
    uint8_t sprite_collide[1024];
    uint8_t mix_collide_summary;
    uint8_t sprite_collide_summary;

    int bank1;          // 8000-bfff ROM bank entry (0..3)
    int videoram_bank;  // from sound_control_w
    int video_mode;     // bit4 blank, bit7 flip
    int flip;

    // Sound subsystem
    uint8_t soundram[0x800];     // 8000-87ff
    uint8_t soundrom[SOUNDROM_SIZE];
    uint8_t soundlatch;
    int sound_nmi_mask;          // port C bit7: 1 = NMI cleared
    int sound_nmi_prev;          // edge detect for NMI assertion
    sn76489 sn1, sn2;            // SN76489A x2 (2MHz and 4MHz)

    // PPI 8255 port C latch (for BSR handling)
    uint8_t ppi_portc;

    // Graphics ROMs / PROMs (read by video.c)
    uint8_t tilerom[TILEROM_SIZE];
    uint8_t spriterom[SPRITEROM_SIZE];
    uint8_t color_prom[0x300];   // R,G,B 256 each
    uint8_t lookup_prom[0x100];

    sys_inputs in;
} system2;

// Returns 0 on success, non-zero if a ROM file is missing.
int machine_init(system2 *m, const char *romdir);

// Run one ~1/60s video frame: executes main + sound CPU cycles (interleaved)
// and fires the VBLANK / sound interrupts, renders into the framebuffer, and
// produces stereo-interleaved? no — mono audio samples into `audio` (one int16
// per sample). Returns the number of samples produced (<= AUDIO_MAX_FRAME).
int machine_run_frame(system2 *m, uint32_t *framebuffer, int16_t *audio);

#endif // MACHINE_H_

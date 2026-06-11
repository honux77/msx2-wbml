// SN76489A PSG — faithful port of MAME sn76496.cpp (SN76489A variant).
#ifndef SN76489_H_
#define SN76489_H_

#include <stdint.h>

typedef struct {
    int      clock_divider;     // 8 for SN76489A
    uint32_t feedback_mask;     // 0x10000
    uint32_t tap1, tap2;        // 0x04, 0x08

    int32_t  channel_clock;     // clock_hz / (2 * clock_divider)
    int32_t  sample_rate;
    int32_t  clock_acc;         // resample accumulator

    int      registers[8];
    int      last_register;
    int      period[4];
    int      count[4];
    int      output[4];
    int      volume[4];         // current per-channel output level
    int      vol_table[16];
    uint32_t rng;
} sn76489;

void sn_init(sn76489 *s, int clock_hz, int sample_rate);
void sn_write(sn76489 *s, uint8_t data);
// Generate `n` mono samples, ADDING (level * gain) into the int32 mix buffer.
void sn_render_add(sn76489 *s, int32_t *mix, int n, double gain);

#endif // SN76489_H_

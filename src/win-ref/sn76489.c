// SN76489A PSG — faithful port of MAME sn76496.cpp (SN76489A variant).
// Texas Instruments 3 square-wave tones + 1 noise channel.
#include "sn76489.h"

#define MAX_OUTPUT 0x7fff

void sn_init(sn76489 *s, int clock_hz, int sample_rate) {
    s->clock_divider = 8;          // SN76489A has the /8 divider
    s->feedback_mask = 0x10000;    // SN76489A LFSR feedback / taps (MAME)
    s->tap1 = 0x04;
    s->tap2 = 0x08;

    s->channel_clock = clock_hz / (2 * s->clock_divider);
    s->sample_rate = sample_rate;
    s->clock_acc = 0;

    for (int i = 0; i < 8; i++) s->registers[i] = 0;
    s->last_register = 0;
    for (int i = 0; i < 4; i++) {
        // Start at the maximum period (not 0): a 0 period would toggle every
        // tick, aliasing into high-frequency noise on the direct resampler
        // before the game programs the channels.
        s->period[i] = 0x400;
        s->count[i] = 0x400;
        s->output[i] = 0;
    }

    // 2dB-per-step volume table, 4 channels share the range
    double out = MAX_OUTPUT / 4;
    for (int i = 0; i < 15; i++) {
        s->vol_table[i] = (out > MAX_OUTPUT / 4) ? MAX_OUTPUT / 4 : (int)out;
        out /= 1.258925412;  // 10^(2/20)
    }
    s->vol_table[15] = 0;

    // Start silenced. Real SN76489A resets volume registers to max, but the
    // game programs all four channels before using them; starting at max here
    // just produces an audible power-on burst, so begin muted.
    for (int i = 0; i < 4; i++) s->volume[i] = s->vol_table[15];

    s->rng = s->feedback_mask;
}

void sn_write(sn76489 *s, uint8_t data) {
    int r;
    if (data & 0x80) {
        r = (data & 0x70) >> 4;
        s->last_register = r;
        s->registers[r] = (s->registers[r] & 0x3f0) | (data & 0x0f);
    } else {
        r = s->last_register;
    }

    int c = r >> 1;
    switch (r) {
        case 0: case 2: case 4:  // tone frequency
            if ((data & 0x80) == 0)
                s->registers[r] = (s->registers[r] & 0x0f) | ((data & 0x3f) << 4);
            s->period[c] = (s->registers[r] != 0) ? s->registers[r] : 0x400;
            if (r == 4 && (s->registers[6] & 3) == 3)
                s->period[3] = s->period[2] << 1;
            break;
        case 1: case 3: case 5: case 7:  // volume
            s->volume[c] = s->vol_table[data & 0x0f];
            if ((data & 0x80) == 0)
                s->registers[r] = (s->registers[r] & 0x3f0) | (data & 0x0f);
            break;
        case 6: {  // noise frequency/mode
            if ((data & 0x80) == 0)
                s->registers[r] = (s->registers[r] & 0x3f0) | (data & 0x0f);
            int n = s->registers[6];
            s->period[3] = ((n & 3) == 3) ? (s->period[2] << 1) : (1 << (5 + (n & 3)));
            s->rng = s->feedback_mask;
            break;
        }
    }
}

static void sn_clock(sn76489 *s) {
    for (int i = 0; i < 3; i++) {
        if (--s->count[i] <= 0) {
            s->output[i] ^= 1;
            s->count[i] = s->period[i];
        }
    }
    if (--s->count[3] <= 0) {
        if (((s->rng & s->tap1) != 0) !=
            (((s->rng & s->tap2) != 0) && ((s->registers[6] >> 2) & 1))) {
            s->rng >>= 1;
            s->rng |= s->feedback_mask;
        } else {
            s->rng >>= 1;
        }
        s->output[3] = s->rng & 1;
        s->count[3] = s->period[3];
    }
}

void sn_render_add(sn76489 *s, int32_t *mix, int n, double gain) {
    for (int i = 0; i < n; i++) {
        // advance the chip by channel_clock/sample_rate ticks (fractional)
        s->clock_acc += s->channel_clock;
        while (s->clock_acc >= s->sample_rate) {
            s->clock_acc -= s->sample_rate;
            sn_clock(s);
        }
        int level = (s->output[0] ? s->volume[0] : 0) +
                    (s->output[1] ? s->volume[1] : 0) +
                    (s->output[2] ? s->volume[2] : 0) +
                    (s->output[3] ? s->volume[3] : 0);
        mix[i] += (int32_t)(level * gain);
    }
}

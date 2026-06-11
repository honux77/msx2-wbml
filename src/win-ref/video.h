// System 2 video renderer (port of MAME system1_v.cpp, system2 path).
#ifndef VIDEO_H_
#define VIDEO_H_

#include <stdint.h>
#include "machine.h"

#define SCREEN_W 512
#define SCREEN_H 224

// Render one full frame into `fb` (SCREEN_W*SCREEN_H, 0xAARRGGBB) and update
// the mixer/sprite collision arrays in `m` (read by the game next frame).
void video_render(system2 *m, uint32_t *fb);

#endif // VIDEO_H_

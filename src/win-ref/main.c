// Windows reference host for wbml: SDL2 window, input, 60Hz loop.
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "machine.h"
#include "video.h"

// Input bit layout (active-high in sys_inputs; inverted on read).
// P1: b1 button2 (jump), b2 button1 (attack), b4 down, b5 up, b6 right, b7 left
// SYSTEM: b0 coin1, b1 coin2, b3 service1, b4 start1, b5 start2
//
// Keys:  arrows = move   Space = attack (button1)   Alt = jump (button2)
//        5 = insert coin   1 = 1P start   2 = 2P start   9 = service   ESC = quit
static void poll_input(system2 *m) {
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    uint8_t p1 = 0, sys = 0;

    if (k[SDL_SCANCODE_LEFT])  p1 |= 0x80;
    if (k[SDL_SCANCODE_RIGHT]) p1 |= 0x40;
    if (k[SDL_SCANCODE_UP])    p1 |= 0x20;
    if (k[SDL_SCANCODE_DOWN])  p1 |= 0x10;
    if (k[SDL_SCANCODE_SPACE]) p1 |= 0x04;  // attack  (button1)
    if (k[SDL_SCANCODE_LALT] || k[SDL_SCANCODE_RALT]) p1 |= 0x02;  // jump (button2)

    if (k[SDL_SCANCODE_5]) sys |= 0x01;  // coin1
    if (k[SDL_SCANCODE_6]) sys |= 0x02;  // coin2
    if (k[SDL_SCANCODE_9]) sys |= 0x08;  // service1
    if (k[SDL_SCANCODE_1]) sys |= 0x10;  // start1
    if (k[SDL_SCANCODE_2]) sys |= 0x20;  // start2

    m->in.p1 = p1;
    m->in.system = sys;
}

int main(int argc, char **argv) {
    const char *romdir = (argc > 1) ? argv[1] : "roms/wbml";

    static system2 m;
    if (machine_init(&m, romdir)) {
        fprintf(stderr, "machine_init failed (check ROM path: %s)\n", romdir);
        return 1;
    }

    // optional sound-register logging for diagnostics: --snlog or SN_LOG=1
    {
        extern int g_sn_log; extern FILE *g_sn_logfile;
        int want_log = (SDL_getenv("SN_LOG") != NULL);
        for (int i = 1; i < argc; i++) if (SDL_strcmp(argv[i], "--snlog") == 0) want_log = 1;
        if (want_log) {
            g_sn_logfile = fopen("build/snlog_live.txt", "w");
            if (g_sn_logfile) { g_sn_log = 1; printf("SN write log -> build/snlog_live.txt\n"); }
        }
    }

    // crisp integer-scaled pixels (nearest-neighbour, no blur)
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = AUDIO_SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    SDL_AudioDeviceID adev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (adev) SDL_PauseAudioDevice(adev, 0);

    // The renderer produces a 512-wide buffer, but that is just the native
    // 256-wide image doubled horizontally (background samples at x/2, sprite
    // pixels are drawn twice). Present at the real 256x224 with square pixels.
    const int DISP_W = SCREEN_W / 2;  // 256
    const int DISP_H = SCREEN_H;      // 224

    int scale = 3;
    SDL_Window *win = SDL_CreateWindow(
        "Wonder Boy: Monster Land - reference",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DISP_W * scale, DISP_H * scale, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(ren, DISP_W, DISP_H);
    SDL_RenderSetIntegerScale(ren, SDL_TRUE);  // keep pixels crisp
    SDL_Texture *tex = SDL_CreateTexture(
        ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        DISP_W, DISP_H);

    static uint32_t fb[SCREEN_W * SCREEN_H];
    static uint32_t disp[256 * 224];  // 256x224 downsampled present buffer
    int running = 1;
    int headless_frames = 0;
    for (int i = 1; i < argc; i++)
        if (SDL_strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
            headless_frames = atoi(argv[i + 1]);

    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 prev = SDL_GetPerformanceCounter();
    double accum = 0.0;
    const double frame_s = 1.0 / 60.06;
    long frame = 0;

    if (!headless_frames) {
        printf("Wonder Boy: Monster Land (reference)\n"
               "  Arrows = move   Space = attack   Alt = jump\n"
               "  5 = insert coin   1 = 1P start   2 = 2P start\n"
               "  9 = service   F12 = screenshot   ESC = quit\n");
        fflush(stdout);
    }

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F12) {
                // dump current framebuffer to a PPM for inspection
                char name[64];
                snprintf(name, sizeof(name), "build/frame_%ld.ppm", frame);
                FILE *f = fopen(name, "wb");
                if (f) {
                    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
                    for (int p = 0; p < SCREEN_W * SCREEN_H; p++) {
                        uint32_t c = fb[p];
                        fputc((c >> 16) & 0xff, f);
                        fputc((c >> 8) & 0xff, f);
                        fputc(c & 0xff, f);
                    }
                    fclose(f);
                    printf("wrote %s\n", name);
                }
            }
        }

        poll_input(&m);
        static int16_t audio[AUDIO_MAX_FRAME];
        int nsamp = machine_run_frame(&m, fb, audio);
        if (adev) {
            // avoid unbounded latency build-up if rendering outpaces playback
            if (SDL_GetQueuedAudioSize(adev) < (Uint32)(AUDIO_SAMPLE_RATE / 4 * sizeof(int16_t)))
                SDL_QueueAudio(adev, audio, nsamp * sizeof(int16_t));
        }
        frame++;

        // downsample 512 -> 256 by taking every other column (the pairs are
        // identical), giving the native image with no blur
        for (int y = 0; y < DISP_H; y++) {
            const uint32_t *src = &fb[y * SCREEN_W];
            uint32_t *dst = &disp[y * DISP_W];
            for (int x = 0; x < DISP_W; x++) dst[x] = src[x * 2];
        }
        SDL_UpdateTexture(tex, NULL, disp, DISP_W * sizeof(uint32_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);

        if (headless_frames && frame >= headless_frames) {
            // write final frame and exit (for automated checks)
            FILE *f = fopen("build/headless.ppm", "wb");
            if (f) {
                fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
                for (int p = 0; p < SCREEN_W * SCREEN_H; p++) {
                    uint32_t c = fb[p];
                    fputc((c >> 16) & 0xff, f);
                    fputc((c >> 8) & 0xff, f);
                    fputc(c & 0xff, f);
                }
                fclose(f);
                printf("headless: wrote build/headless.ppm after %ld frames\n", frame);
            }
            running = 0;
        }

        // Pace to ~60.06Hz. Cooperates with vsync: whichever waits longer wins,
        // so this caps refresh on >60Hz monitors and keeps audio from starving.
        (void)accum;
        if (headless_frames) continue;  // run flat-out for automated capture
        double target_ticks = frame_s * (double)freq;
        for (;;) {
            Uint64 now = SDL_GetPerformanceCounter();
            double elapsed = (double)(now - prev);
            if (elapsed >= target_ticks) { prev = now; break; }
            double remain_ms = (target_ticks - elapsed) * 1000.0 / (double)freq;
            if (remain_ms > 2.0) SDL_Delay((Uint32)(remain_ms - 1.0));
        }
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

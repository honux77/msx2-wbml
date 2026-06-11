// Windows reference host for wbml: SDL2 window, input, 60Hz loop.
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "machine.h"
#include "video.h"

// Input bit layout (active-high in sys_inputs; inverted on read).
// P1: b1 button2, b2 button1, b4 down, b5 up, b6 right, b7 left
// SYSTEM: b0 coin1, b1 coin2, b3 service1, b4 start1, b5 start2
static void poll_input(system2 *m) {
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    uint8_t p1 = 0, sys = 0;

    if (k[SDL_SCANCODE_LEFT])  p1 |= 0x80;
    if (k[SDL_SCANCODE_RIGHT]) p1 |= 0x40;
    if (k[SDL_SCANCODE_UP])    p1 |= 0x20;
    if (k[SDL_SCANCODE_DOWN])  p1 |= 0x10;
    if (k[SDL_SCANCODE_Z])     p1 |= 0x04;  // button1
    if (k[SDL_SCANCODE_X])     p1 |= 0x02;  // button2

    if (k[SDL_SCANCODE_5] || k[SDL_SCANCODE_C]) sys |= 0x01;  // coin1
    if (k[SDL_SCANCODE_9])                       sys |= 0x08;  // service1
    if (k[SDL_SCANCODE_1])                       sys |= 0x10;  // start1
    if (k[SDL_SCANCODE_2])                       sys |= 0x20;  // start2

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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    int scale = 3;
    SDL_Window *win = SDL_CreateWindow(
        "Wonder Boy: Monster Land - reference",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * scale, SCREEN_H * scale, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(ren, SCREEN_W, SCREEN_H);
    SDL_Texture *tex = SDL_CreateTexture(
        ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W, SCREEN_H);

    static uint32_t fb[SCREEN_W * SCREEN_H];
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
        machine_run_frame(&m, fb);
        frame++;

        SDL_UpdateTexture(tex, NULL, fb, SCREEN_W * sizeof(uint32_t));
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

        // pace to ~60Hz when vsync is unavailable
        (void)accum; (void)frame_s; (void)freq;
        Uint64 now = SDL_GetPerformanceCounter();
        prev = now;
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

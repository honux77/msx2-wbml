// Windows reference host for wbml: SDL2 window, config menu, input, 60Hz loop.
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "machine.h"
#include "video.h"
#include "cfg.h"
#include "menu.h"

static void poll_input(system2 *m, const wbml_cfg *cfg, SDL_Joystick *joy) {
    static unsigned turbo_tick = 0;
    turbo_tick++;

    const Uint8 *k = SDL_GetKeyboardState(NULL);
    uint8_t p1 = 0, sys = 0;

    if (k[cfg->k_left])   p1 |= 0x80;
    if (k[cfg->k_right])  p1 |= 0x40;
    if (k[cfg->k_up])     p1 |= 0x20;
    if (k[cfg->k_down])   p1 |= 0x10;
    if (k[cfg->k_jump])   p1 |= 0x02;
    if (k[cfg->k_attack]) p1 |= 0x04;
    if (k[cfg->k_turbo])  {
        if ((turbo_tick >> 1) & 1) p1 |= 0x80; else p1 |= 0x40;
    }
    if (k[cfg->k_coin])   sys |= 0x01;
    if (k[cfg->k_start1]) sys |= 0x10;

    if (joy) {
        Sint16 ax = SDL_JoystickGetAxis(joy, cfg->joy_axis_x);
        Sint16 ay = SDL_JoystickGetAxis(joy, cfg->joy_axis_y);
        if (ax < -8000) p1 |= 0x80;
        if (ax >  8000) p1 |= 0x40;
        if (ay < -8000) p1 |= 0x20;
        if (ay >  8000) p1 |= 0x10;
        if (SDL_JoystickGetButton(joy, cfg->joy_btn_jump))   p1 |= 0x02;
        if (SDL_JoystickGetButton(joy, cfg->joy_btn_attack)) p1 |= 0x04;
    }

    m->in.p1  = p1;
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

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    // Load config (SDL_GetPrefPath gives a writable per-user directory)
    static char cfg_path[512];
    {
        char *pref = SDL_GetPrefPath("wbml", "wbml");
        if (pref) {
            snprintf(cfg_path, sizeof(cfg_path), "%swbml.cfg", pref);
            SDL_free(pref);
        } else {
            SDL_strlcpy(cfg_path, "wbml.cfg", sizeof(cfg_path));
        }
    }
    wbml_cfg cfg;
    cfg_defaults(&cfg);
    cfg_load(&cfg, cfg_path);

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = AUDIO_SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    SDL_AudioDeviceID adev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (adev) SDL_PauseAudioDevice(adev, 0);

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
    SDL_RenderSetIntegerScale(ren, SDL_TRUE);
    SDL_Texture *tex = SDL_CreateTexture(
        ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        DISP_W, DISP_H);

    // Show config menu before game
    int headless_frames = 0;
    for (int i = 1; i < argc; i++)
        if (SDL_strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
            headless_frames = atoi(argv[i + 1]);

    if (!headless_frames) {
        if (!run_menu(ren, &cfg, cfg_path)) {
            SDL_DestroyTexture(tex);
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            SDL_Quit();
            return 0;
        }
    }

    // Open joystick if configured
    SDL_Joystick *joy = NULL;
    if (cfg.joy_index >= 0 && cfg.joy_index < SDL_NumJoysticks())
        joy = SDL_JoystickOpen(cfg.joy_index);

    static uint32_t fb[SCREEN_W * SCREEN_H];
    static uint32_t disp[256 * 224];
    int running = 1;

    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 prev = SDL_GetPerformanceCounter();
    const double frame_s = 1.0 / 60.06;
    long frame = 0;

    printf("Wonder Boy: Monster Land (reference)\n");
    printf("  Z=jump  X=attack  C=turbo-LR\n");
    printf("  5=coin  1=start   F12=screenshot  ESC=quit\n");
    fflush(stdout);

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F12) {
                char name[64];
                snprintf(name, sizeof(name), "build/frame_%ld.ppm", frame);
                FILE *f = fopen(name, "wb");
                if (f) {
                    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
                    for (int p = 0; p < SCREEN_W * SCREEN_H; p++) {
                        uint32_t c = fb[p];
                        fputc((c >> 16) & 0xff, f);
                        fputc((c >>  8) & 0xff, f);
                        fputc( c        & 0xff, f);
                    }
                    fclose(f);
                    printf("wrote %s\n", name);
                }
            }
        }

        poll_input(&m, &cfg, joy);
        static int16_t audio[AUDIO_MAX_FRAME];
        int nsamp = machine_run_frame(&m, fb, audio);
        if (adev) {
            if (SDL_GetQueuedAudioSize(adev) < (Uint32)(AUDIO_SAMPLE_RATE / 4 * sizeof(int16_t)))
                SDL_QueueAudio(adev, audio, nsamp * sizeof(int16_t));
        }
        frame++;

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
            FILE *f = fopen("build/headless.ppm", "wb");
            if (f) {
                fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
                for (int p = 0; p < SCREEN_W * SCREEN_H; p++) {
                    uint32_t c = fb[p];
                    fputc((c >> 16) & 0xff, f);
                    fputc((c >>  8) & 0xff, f);
                    fputc( c        & 0xff, f);
                }
                fclose(f);
                printf("headless: wrote build/headless.ppm after %ld frames\n", frame);
            }
            running = 0;
        }

        if (headless_frames) continue;
        double target_ticks = frame_s * (double)freq;
        for (;;) {
            Uint64 now = SDL_GetPerformanceCounter();
            double elapsed = (double)(now - prev);
            if (elapsed >= target_ticks) { prev = now; break; }
            double remain_ms = (target_ticks - elapsed) * 1000.0 / (double)freq;
            if (remain_ms > 2.0) SDL_Delay((Uint32)(remain_ms - 1.0));
        }
    }

    if (joy) SDL_JoystickClose(joy);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

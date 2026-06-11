// Config menu with embedded 8x8 bitmap font (public domain font8x8_basic)
#include "menu.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// 8x8 bitmap font — printable ASCII 0x20-0x7E (95 glyphs)
// Each byte is one row; MSB = leftmost pixel.
// Source: font8x8_basic (public domain)
// ---------------------------------------------------------------------------
static const uint8_t g_font[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 0x20 space
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 0x21 !
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, // 0x22 "
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // 0x23 #
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // 0x24 $
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // 0x25 %
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, // 0x26 &
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, // 0x27 '
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, // 0x28 (
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, // 0x29 )
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 0x2A *
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, // 0x2B +
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, // 0x2C ,
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, // 0x2D -
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, // 0x2E .
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00}, // 0x2F /
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, // 0x30 0
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, // 0x31 1
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, // 0x32 2
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00}, // 0x33 3
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, // 0x34 4
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00}, // 0x35 5
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, // 0x36 6
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00}, // 0x37 7
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, // 0x38 8
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00}, // 0x39 9
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, // 0x3A :
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06}, // 0x3B ;
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00}, // 0x3C <
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00}, // 0x3D =
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, // 0x3E >
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00}, // 0x3F ?
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00}, // 0x40 @
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00}, // 0x41 A
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00}, // 0x42 B
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00}, // 0x43 C
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00}, // 0x44 D
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00}, // 0x45 E
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00}, // 0x46 F
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00}, // 0x47 G
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00}, // 0x48 H
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 0x49 I
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00}, // 0x4A J
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00}, // 0x4B K
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00}, // 0x4C L
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00}, // 0x4D M
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, // 0x4E N
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00}, // 0x4F O
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00}, // 0x50 P
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00}, // 0x51 Q
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00}, // 0x52 R
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00}, // 0x53 S
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 0x54 T
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00}, // 0x55 U
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 0x56 V
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 0x57 W
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00}, // 0x58 X
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00}, // 0x59 Y
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00}, // 0x5A Z
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00}, // 0x5B [
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00}, // 0x5C backslash
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00}, // 0x5D ]
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00}, // 0x5E ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // 0x5F _
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00}, // 0x60 `
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00}, // 0x61 a
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00}, // 0x62 b
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00}, // 0x63 c
    {0x38,0x30,0x30,0x3E,0x33,0x33,0x6E,0x00}, // 0x64 d
    {0x00,0x00,0x1E,0x33,0x3F,0x03,0x1E,0x00}, // 0x65 e
    {0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00}, // 0x66 f
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F}, // 0x67 g
    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00}, // 0x68 h
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00}, // 0x69 i
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E}, // 0x6A j
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00}, // 0x6B k
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 0x6C l
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00}, // 0x6D m
    {0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00}, // 0x6E n
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00}, // 0x6F o
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F}, // 0x70 p
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78}, // 0x71 q
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00}, // 0x72 r
    {0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00}, // 0x73 s
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00}, // 0x74 t
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00}, // 0x75 u
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 0x76 v
    {0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00}, // 0x77 w
    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00}, // 0x78 x
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F}, // 0x79 y
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00}, // 0x7A z
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00}, // 0x7B {
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 0x7C |
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00}, // 0x7D }
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00}, // 0x7E ~
};

// ---------------------------------------------------------------------------
// Rendering helpers (logical 256x224 coordinates)
// ---------------------------------------------------------------------------
typedef struct { uint8_t r, g, b; } Col;
static const Col WHITE  = {255,255,255};
static const Col YELLOW = {255,220,  0};
static const Col GRAY   = {128,128,128};
static const Col CYAN   = {  0,220,255};
static const Col RED    = {255, 80, 80};

static void set_color(SDL_Renderer *r, Col c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
}

static void draw_char(SDL_Renderer *r, int x, int y, unsigned char c, Col col) {
    if (c < 0x20 || c > 0x7E) c = '?';
    const uint8_t *g = g_font[c - 0x20];
    set_color(r, col);
    for (int row = 0; row < 8; row++) {
        uint8_t bits = g[row];
        for (int col2 = 0; col2 < 8; col2++)
            if (bits & (0x80 >> col2))
                SDL_RenderDrawPoint(r, x + col2, y + row);
    }
}

static void draw_str(SDL_Renderer *r, int x, int y, const char *s, Col col) {
    for (; *s; s++, x += 8) draw_char(r, x, y, (unsigned char)*s, col);
}

// Draw text centred at cx (pixel coordinate)
static void draw_str_c(SDL_Renderer *r, int cx, int y, const char *s, Col col) {
    draw_str(r, cx - (int)(strlen(s) * 4), y, s, col);
}

static void fill_rect(SDL_Renderer *r, int x, int y, int w, int h, Col col) {
    SDL_Rect rc = {x, y, w, h};
    set_color(r, col);
    SDL_RenderFillRect(r, &rc);
}

// Character grid helpers: each cell is 8x8 px on the 256x224 logical surface
#define CX(col)  ((col) * 8)
#define CY(row)  ((row) * 8)
#define COLS 32
#define ROWS 28

// ---------------------------------------------------------------------------
// Menu state machine
// ---------------------------------------------------------------------------
typedef enum { MS_MAIN, MS_KEYS, MS_JOY } MenuScreen;

// Action names for keyboard settings
static const char *k_labels[] = {
    "LEFT", "RIGHT", "UP", "DOWN",
    "JUMP", "ATTACK", "TURBO", "COIN", "START1"
};
static SDL_Scancode *k_fields(wbml_cfg *c, int i) {
    SDL_Scancode *arr[] = {
        &c->k_left, &c->k_right, &c->k_up, &c->k_down,
        &c->k_jump, &c->k_attack, &c->k_turbo,
        &c->k_coin, &c->k_start1
    };
    return arr[i];
}
#define NUM_KEYS 9

// Joystick names for display
static const char *joy_row_labels[] = {
    "DEVICE", "JUMP BTN", "ATTACK BTN", "H-AXIS", "V-AXIS"
};
#define NUM_JOY_ROWS 5

// Open all connected joysticks for event detection
static SDL_Joystick *g_joys[16];
static int g_njoys = 0;

static void open_all_joysticks(void) {
    g_njoys = SDL_NumJoysticks();
    if (g_njoys > 16) g_njoys = 16;
    for (int i = 0; i < g_njoys; i++)
        g_joys[i] = SDL_JoystickOpen(i);
}

static void close_all_joysticks(void) {
    for (int i = 0; i < g_njoys; i++)
        if (g_joys[i]) { SDL_JoystickClose(g_joys[i]); g_joys[i] = NULL; }
    g_njoys = 0;
}

// Present the current frame
static void present(SDL_Renderer *r) { SDL_RenderPresent(r); }

// Draw a horizontal separator line
static void draw_sep(SDL_Renderer *r, int row) {
    set_color(r, GRAY);
    SDL_RenderDrawLine(r, 0, CY(row), 255, CY(row));
}

// Draw header + footer chrome shared by all screens
static void draw_chrome(SDL_Renderer *r, const char *title) {
    // background
    SDL_SetRenderDrawColor(r, 8, 8, 48, 255);
    SDL_RenderClear(r);
    // title bar
    fill_rect(r, 0, 0, 256, 10, (Col){0,0,80});
    draw_str_c(r, 128, 1, title, YELLOW);
    draw_sep(r, 2);
    // footer
    draw_sep(r, ROWS - 3);
    draw_str_c(r, 128, CY(ROWS-2), "UP/DN:SELECT  ENTER:OK  ESC:BACK", GRAY);
}

// ---------------------------------------------------------------------------
// MAIN MENU
// ---------------------------------------------------------------------------
static const char *main_items[] = {
    "KEYBOARD SETTINGS",
    "JOYSTICK SETTINGS",
    "START GAME",
    "QUIT"
};
#define MAIN_N 4

static void draw_main(SDL_Renderer *r, int sel) {
    draw_chrome(r, "WONDER BOY: MONSTER LAND");
    int y0 = CY(5);
    for (int i = 0; i < MAIN_N; i++) {
        int y = y0 + i * 18;
        Col c = (i == sel) ? YELLOW : WHITE;
        if (i == sel) draw_str(r, CX(2), y, ">", YELLOW);
        draw_str(r, CX(4), y, main_items[i], c);
    }
}

// ---------------------------------------------------------------------------
// KEYBOARD SETTINGS
// ---------------------------------------------------------------------------
static void draw_keys(SDL_Renderer *r, wbml_cfg *cfg, int sel, int rebinding) {
    draw_chrome(r, "KEYBOARD SETTINGS");
    int y0 = CY(3);
    for (int i = 0; i < NUM_KEYS; i++) {
        int y = y0 + i * 17;
        int is_sel = (i == sel);
        Col lc = is_sel ? YELLOW : WHITE;
        Col vc = is_sel ? CYAN   : GRAY;
        if (is_sel) draw_str(r, CX(1), y, ">", YELLOW);
        char buf[32];
        snprintf(buf, sizeof(buf), "%-8s", k_labels[i]);
        draw_str(r, CX(2), y, buf, lc);
        const char *kn = SDL_GetScancodeName(*k_fields(cfg, i));
        if (rebinding && is_sel)
            draw_str(r, CX(13), y, "Press a key...", RED);
        else
            draw_str(r, CX(13), y, kn, vc);
    }
    // BACK item
    int y_back = y0 + NUM_KEYS * 17;
    Col bc = (sel == NUM_KEYS) ? YELLOW : WHITE;
    if (sel == NUM_KEYS) draw_str(r, CX(1), y_back, ">", YELLOW);
    draw_str(r, CX(2), y_back, "BACK", bc);
}

// ---------------------------------------------------------------------------
// JOYSTICK SETTINGS
// ---------------------------------------------------------------------------
static void draw_joy_row(SDL_Renderer *r, wbml_cfg *cfg, int i, int y,
                         int is_sel, int detecting) {
    Col lc = is_sel ? YELLOW : WHITE;
    Col vc = is_sel ? CYAN   : GRAY;
    if (is_sel) draw_str(r, CX(1), y, ">", YELLOW);
    char lbuf[16];
    snprintf(lbuf, sizeof(lbuf), "%-10s", joy_row_labels[i]);
    draw_str(r, CX(2), y, lbuf, lc);

    if (detecting && is_sel) {
        draw_str(r, CX(13), y, "...", RED);
        return;
    }

    char vbuf[32];
    switch (i) {
    case 0: // DEVICE
        if (cfg->joy_index < 0) {
            snprintf(vbuf, sizeof(vbuf), "DISABLED");
        } else {
            const char *name = (cfg->joy_index < g_njoys && g_joys[cfg->joy_index])
                ? SDL_JoystickName(g_joys[cfg->joy_index]) : "?";
            snprintf(vbuf, sizeof(vbuf), "[%d] %.14s", cfg->joy_index, name);
        }
        break;
    case 1: snprintf(vbuf, sizeof(vbuf), "Button %d", cfg->joy_btn_jump);   break;
    case 2: snprintf(vbuf, sizeof(vbuf), "Button %d", cfg->joy_btn_attack); break;
    case 3: snprintf(vbuf, sizeof(vbuf), "Axis %d",   cfg->joy_axis_x);     break;
    case 4: snprintf(vbuf, sizeof(vbuf), "Axis %d",   cfg->joy_axis_y);     break;
    }
    draw_str(r, CX(13), y, vbuf, vc);
}

static void draw_joy(SDL_Renderer *r, wbml_cfg *cfg, int sel, int detecting) {
    draw_chrome(r, "JOYSTICK SETTINGS");
    if (g_njoys == 0)
        draw_str_c(r, 128, CY(3), "No joystick detected", RED);
    else {
        char info[32];
        snprintf(info, sizeof(info), "%d joystick(s) found", g_njoys);
        draw_str_c(r, 128, CY(3), info, GRAY);
    }

    int y0 = CY(5);
    for (int i = 0; i < NUM_JOY_ROWS; i++)
        draw_joy_row(r, cfg, i, y0 + i * 17, i == sel, detecting);

    // BACK
    int y_back = y0 + NUM_JOY_ROWS * 17;
    Col bc = (sel == NUM_JOY_ROWS) ? YELLOW : WHITE;
    if (sel == NUM_JOY_ROWS) draw_str(r, CX(1), y_back, ">", YELLOW);
    draw_str(r, CX(2), y_back, "BACK", bc);
}

// Hint for joystick rows
static void draw_joy_hint(SDL_Renderer *r, int sel) {
    const char *hints[] = {
        "L/R:CHANGE DEVICE",
        "ENTER:PRESS BUTTON",
        "ENTER:PRESS BUTTON",
        "ENTER:MOVE L/R",
        "ENTER:MOVE UP/DN",
    };
    if (sel < NUM_JOY_ROWS) {
        draw_str_c(r, 128, CY(ROWS-2), hints[sel], GRAY);
    }
}

// ---------------------------------------------------------------------------
// Main menu entry point
// ---------------------------------------------------------------------------
int run_menu(SDL_Renderer *ren, wbml_cfg *cfg, const char *cfg_path) {
    open_all_joysticks();

    MenuScreen screen = MS_MAIN;
    int sel = 0;
    int result = 0;  // 0=quit, 1=start
    int rebinding = 0;
    int detecting = 0;
    int running = 1;

    while (running) {
        // Render
        switch (screen) {
        case MS_MAIN: draw_main(ren, sel); break;
        case MS_KEYS: draw_keys(ren, cfg, sel, rebinding); break;
        case MS_JOY:  draw_joy (ren, cfg, sel, detecting);
                      if (!detecting) draw_joy_hint(ren, sel);
                      break;
        }
        present(ren);

        // Handle events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = 0; result = 0; break; }

            // --- Keyboard rebind mode ---
            if (rebinding && screen == MS_KEYS && e.type == SDL_KEYDOWN) {
                SDL_Scancode sc = e.key.keysym.scancode;
                if (sc == SDL_SCANCODE_ESCAPE) {
                    // cancel
                } else if (sc != SDL_SCANCODE_RETURN) {
                    *k_fields(cfg, sel) = sc;
                    cfg_save(cfg, cfg_path);
                }
                rebinding = 0;
                continue;
            }

            // --- Joystick detect mode ---
            if (detecting && screen == MS_JOY) {
                if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    detecting = 0; continue;
                }
                if (sel == 1 && e.type == SDL_JOYBUTTONDOWN) {
                    cfg->joy_btn_jump   = e.jbutton.button;
                    cfg_save(cfg, cfg_path); detecting = 0; continue;
                }
                if (sel == 2 && e.type == SDL_JOYBUTTONDOWN) {
                    cfg->joy_btn_attack = e.jbutton.button;
                    cfg_save(cfg, cfg_path); detecting = 0; continue;
                }
                if ((sel == 3 || sel == 4) && e.type == SDL_JOYAXISMOTION) {
                    if (e.jaxis.value > 16000 || e.jaxis.value < -16000) {
                        if (sel == 3) cfg->joy_axis_x = e.jaxis.axis;
                        else          cfg->joy_axis_y = e.jaxis.axis;
                        cfg_save(cfg, cfg_path); detecting = 0; continue;
                    }
                }
                continue; // swallow other events while detecting
            }

            // --- Normal navigation ---
            if (e.type != SDL_KEYDOWN) continue;
            SDL_Scancode sc = e.key.keysym.scancode;

            int max_sel = (screen == MS_MAIN) ? MAIN_N - 1
                        : (screen == MS_KEYS) ? NUM_KEYS
                        :                       NUM_JOY_ROWS;

            if (sc == SDL_SCANCODE_UP)   { sel = (sel > 0) ? sel - 1 : max_sel; continue; }
            if (sc == SDL_SCANCODE_DOWN) { sel = (sel < max_sel) ? sel + 1 : 0; continue; }

            if (sc == SDL_SCANCODE_ESCAPE) {
                if (screen != MS_MAIN) { screen = MS_MAIN; sel = 0; }
                continue;
            }

            if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_KP_ENTER) {
                if (screen == MS_MAIN) {
                    switch (sel) {
                    case 0: screen = MS_KEYS; sel = 0; break;
                    case 1: screen = MS_JOY;  sel = 0; break;
                    case 2: running = 0; result = 1; break;
                    case 3: running = 0; result = 0; break;
                    }
                } else if (screen == MS_KEYS) {
                    if (sel == NUM_KEYS) { screen = MS_MAIN; sel = 0; }
                    else                 { rebinding = 1; }
                } else if (screen == MS_JOY) {
                    if (sel == NUM_JOY_ROWS) { screen = MS_MAIN; sel = 0; }
                    else if (sel == 0) { /* handled by L/R below */ }
                    else               { detecting = 1; }
                }
                continue;
            }

            // Joystick device selection with left/right
            if (screen == MS_JOY && sel == 0) {
                if (sc == SDL_SCANCODE_LEFT) {
                    cfg->joy_index--;
                    if (cfg->joy_index < -1) cfg->joy_index = g_njoys - 1;
                    cfg_save(cfg, cfg_path);
                }
                if (sc == SDL_SCANCODE_RIGHT) {
                    cfg->joy_index++;
                    if (cfg->joy_index >= g_njoys) cfg->joy_index = -1;
                    cfg_save(cfg, cfg_path);
                }
            }
        }
    }

    close_all_joysticks();
    return result;
}

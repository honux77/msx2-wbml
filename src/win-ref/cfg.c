#include "cfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cfg_defaults(wbml_cfg *c) {
    c->k_left   = SDL_SCANCODE_LEFT;
    c->k_right  = SDL_SCANCODE_RIGHT;
    c->k_up     = SDL_SCANCODE_UP;
    c->k_down   = SDL_SCANCODE_DOWN;
    c->k_jump   = SDL_SCANCODE_Z;
    c->k_attack = SDL_SCANCODE_X;
    c->k_turbo  = SDL_SCANCODE_C;
    c->k_coin   = SDL_SCANCODE_5;
    c->k_start1 = SDL_SCANCODE_1;
    c->joy_index    = -1;
    c->joy_axis_x   = 0;
    c->joy_axis_y   = 1;
    c->joy_btn_jump   = 0;
    c->joy_btn_attack = 1;
}

int cfg_load(wbml_cfg *c, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char key[64], val[64];
    while (fscanf(f, " %63[^=]=%63[^\n]", key, val) == 2) {
        SDL_Scancode sc = SDL_GetScancodeFromName(val);
#define TRY_SC(field) \
        if (!strcmp(key, #field)) { if (sc != SDL_SCANCODE_UNKNOWN) c->field = sc; } else
        TRY_SC(k_left) TRY_SC(k_right) TRY_SC(k_up) TRY_SC(k_down)
        TRY_SC(k_jump) TRY_SC(k_attack) TRY_SC(k_turbo)
        TRY_SC(k_coin) TRY_SC(k_start1)
#undef TRY_SC
        if      (!strcmp(key, "joy_index"))     c->joy_index      = atoi(val);
        else if (!strcmp(key, "joy_axis_x"))    c->joy_axis_x     = atoi(val);
        else if (!strcmp(key, "joy_axis_y"))    c->joy_axis_y     = atoi(val);
        else if (!strcmp(key, "joy_btn_jump"))   c->joy_btn_jump   = atoi(val);
        else if (!strcmp(key, "joy_btn_attack")) c->joy_btn_attack = atoi(val);
    }
    fclose(f);
    return 1;
}

void cfg_save(const wbml_cfg *c, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "k_left=%s\nk_right=%s\nk_up=%s\nk_down=%s\n",
            SDL_GetScancodeName(c->k_left),  SDL_GetScancodeName(c->k_right),
            SDL_GetScancodeName(c->k_up),    SDL_GetScancodeName(c->k_down));
    fprintf(f, "k_jump=%s\nk_attack=%s\nk_turbo=%s\nk_coin=%s\nk_start1=%s\n",
            SDL_GetScancodeName(c->k_jump),  SDL_GetScancodeName(c->k_attack),
            SDL_GetScancodeName(c->k_turbo), SDL_GetScancodeName(c->k_coin),
            SDL_GetScancodeName(c->k_start1));
    fprintf(f, "joy_index=%d\njoy_axis_x=%d\njoy_axis_y=%d\n",
            c->joy_index, c->joy_axis_x, c->joy_axis_y);
    fprintf(f, "joy_btn_jump=%d\njoy_btn_attack=%d\n",
            c->joy_btn_jump, c->joy_btn_attack);
    fclose(f);
}

#pragma once
#include <SDL.h>

typedef struct {
    SDL_Scancode k_left, k_right, k_up, k_down;
    SDL_Scancode k_jump, k_attack, k_turbo;
    SDL_Scancode k_coin, k_start1;
    int joy_index;      // -1 = disabled
    int joy_axis_x, joy_axis_y;
    int joy_btn_jump, joy_btn_attack;
} wbml_cfg;

void cfg_defaults(wbml_cfg *c);
int  cfg_load(wbml_cfg *c, const char *path);
void cfg_save(const wbml_cfg *c, const char *path);

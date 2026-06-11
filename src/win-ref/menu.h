#pragma once
#include <SDL.h>
#include "cfg.h"

// Show the config menu. Returns 1 = start game, 0 = quit.
int run_menu(SDL_Renderer *ren, wbml_cfg *cfg, const char *cfg_path);

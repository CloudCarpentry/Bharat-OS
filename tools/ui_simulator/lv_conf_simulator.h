/* SPDX-License-Identifier: MIT */
#ifndef LV_CONF_SIMULATOR_H
#define LV_CONF_SIMULATOR_H

/* Include the base Bharat configuration */
#include "lv_conf_bharat.h"

/* Enable SDL for simulator */
#define LV_USE_SDL              1
/* #define LV_SDL_INCLUDE_PATH     <SDL2/SDL.h> */ /* Let CMake handle includes */

#endif /*LV_CONF_SIMULATOR_H*/

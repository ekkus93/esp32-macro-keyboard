#ifndef DEVICE_CONTROLS_LOGIC_H
#define DEVICE_CONTROLS_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#include "device_controls.h"

#define DEVICE_CONTROLS_DEBOUNCE_SAMPLES 3U

typedef struct {
    bool stable;
    bool candidate;
    uint8_t candidate_count;
} device_controls_debounce_t;

bool device_controls_level_is_pressed(int level, int active_level);
bool device_controls_debounce_update(device_controls_debounce_t *button, bool sample);
bool device_controls_indicator_on(device_indicator_state_t state, uint32_t elapsed_ms);

#endif

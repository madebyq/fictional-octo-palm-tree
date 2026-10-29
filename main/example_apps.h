#ifndef EXAMPLE_APPS_H
#define EXAMPLE_APPS_H

#include "round_core.h"

#define EXAMPLE_APP_COUNT 3

typedef struct {
    round_agent_state_t agent;
    bool solar_available;
    uint32_t solar_watts;
    uint8_t battery_percent;
} example_state_t;

void example_apps_init(example_state_t *state, round_app_t apps[EXAMPLE_APP_COUNT]);

#endif

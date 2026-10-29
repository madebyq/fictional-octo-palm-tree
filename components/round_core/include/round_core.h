#ifndef ROUND_CORE_H
#define ROUND_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    ROUND_AGENT_IDLE,
    ROUND_AGENT_WORKING,
    ROUND_AGENT_ATTENTION,
    ROUND_AGENT_COMPLETE,
    ROUND_AGENT_SURPRISE
} round_agent_state_t;

typedef enum {
    ROUND_EVENT_NEXT,
    ROUND_EVENT_PREVIOUS,
    ROUND_EVENT_AGENT,
    ROUND_EVENT_SOLAR
} round_event_type_t;

typedef struct {
    round_event_type_t type;
    union {
        round_agent_state_t agent;
        struct {
            uint32_t watts;
            uint8_t battery_percent;
        } solar;
    } data;
} round_event_t;

typedef struct {
    char title[32];
    char body[128];
    char footer[64];
} round_view_t;

typedef struct {
    const char *id;
    void *context;
    void (*on_event)(void *context, const round_event_t *event);
    void (*render)(const void *context, round_view_t *view);
} round_app_t;

typedef struct {
    const round_app_t *apps;
    size_t count;
    size_t active;
} round_runtime_t;

/* App descriptors and contexts must outlive the runtime. Use a single owner task. */
bool round_init(round_runtime_t *runtime, const round_app_t *apps, size_t count);
bool round_dispatch(round_runtime_t *runtime, const round_event_t *event);
bool round_render(const round_runtime_t *runtime, round_view_t *view);

#endif

#include "example_apps.h"

#include <inttypes.h>
#include <stdio.h>

static void home_render(const void *context, round_view_t *view)
{
    (void)context;
    snprintf(view->title, sizeof(view->title), "Round Foundation");
    snprintf(view->body, sizeof(view->body), "Home\nAgent companion\nSolar dashboard");
    snprintf(view->footer, sizeof(view->footer), "Tap Next to switch apps");
}

static void agent_event(void *context, const round_event_t *event)
{
    example_state_t *state = context;
    if (event->type == ROUND_EVENT_AGENT) {
        state->agent = event->data.agent;
    }
}

static void agent_render(const void *context, round_view_t *view)
{
    static const char *const names[] = {"Idle", "Working", "Attention", "Complete", "Surprise"};
    const example_state_t *state = context;
    snprintf(view->title, sizeof(view->title), "Agent companion");
    snprintf(view->body, sizeof(view->body), "%s", names[state->agent]);
    snprintf(view->footer, sizeof(view->footer), "Local status - no AI runs on device");
}

static void solar_event(void *context, const round_event_t *event)
{
    example_state_t *state = context;
    if (event->type == ROUND_EVENT_SOLAR) {
        state->solar_available = true;
        state->solar_watts = event->data.solar.watts;
        state->battery_percent = event->data.solar.battery_percent;
    }
}

static void solar_render(const void *context, round_view_t *view)
{
    const example_state_t *state = context;
    snprintf(view->title, sizeof(view->title), "Solar dashboard");
    if (state->solar_available) {
        snprintf(view->body, sizeof(view->body), "Generation: %" PRIu32 " W\nBattery: %u%%",
                 state->solar_watts, (unsigned)state->battery_percent);
        snprintf(view->footer, sizeof(view->footer), "Last received sample - not live");
    } else {
        snprintf(view->body, sizeof(view->body), "Waiting for telemetry");
        snprintf(view->footer, sizeof(view->footer), "No simulated readings");
    }
}

void example_apps_init(example_state_t *state, round_app_t apps[EXAMPLE_APP_COUNT])
{
    *state = (example_state_t){.agent = ROUND_AGENT_IDLE};
    apps[0] = (round_app_t){.id = "home", .render = home_render};
    apps[1] = (round_app_t){
        .id = "agent", .context = state, .on_event = agent_event, .render = agent_render
    };
    apps[2] = (round_app_t){
        .id = "solar", .context = state, .on_event = solar_event, .render = solar_render
    };
}

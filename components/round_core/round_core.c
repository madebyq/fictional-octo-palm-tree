#include "round_core.h"

#include <string.h>

bool round_init(round_runtime_t *runtime, const round_app_t *apps, size_t count)
{
    if (!runtime) {
        return false;
    }
    *runtime = (round_runtime_t){0};
    if (!apps || count == 0) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        if (!apps[i].id || !apps[i].id[0] || !apps[i].render) {
            return false;
        }
        for (size_t j = 0; j < i; ++j) {
            if (strcmp(apps[i].id, apps[j].id) == 0) {
                return false;
            }
        }
    }
    *runtime = (round_runtime_t){.apps = apps, .count = count};
    return true;
}

bool round_dispatch(round_runtime_t *runtime, const round_event_t *event)
{
    if (!runtime || !runtime->apps || !runtime->count || !event) {
        return false;
    }
    switch (event->type) {
    case ROUND_EVENT_NEXT:
        runtime->active = (runtime->active + 1) % runtime->count;
        return true;
    case ROUND_EVENT_PREVIOUS:
        runtime->active = runtime->active == 0 ? runtime->count - 1 : runtime->active - 1;
        return true;
    case ROUND_EVENT_AGENT:
        if (event->data.agent < ROUND_AGENT_IDLE || event->data.agent > ROUND_AGENT_SURPRISE) {
            return false;
        }
        break;
    case ROUND_EVENT_SOLAR:
        if (event->data.solar.battery_percent > 100) {
            return false;
        }
        break;
    default:
        return false;
    }
    /* Update inactive apps too, so navigation never loses a backend update. */
    for (size_t i = 0; i < runtime->count; ++i) {
        if (runtime->apps[i].on_event) {
            runtime->apps[i].on_event(runtime->apps[i].context, event);
        }
    }
    return true;
}

bool round_render(const round_runtime_t *runtime, round_view_t *view)
{
    if (!runtime || !runtime->apps || runtime->active >= runtime->count || !view) {
        return false;
    }
    memset(view, 0, sizeof(*view));
    const round_app_t *app = &runtime->apps[runtime->active];
    app->render(app->context, view);
    view->title[sizeof(view->title) - 1] = '\0';
    view->body[sizeof(view->body) - 1] = '\0';
    view->footer[sizeof(view->footer) - 1] = '\0';
    return true;
}

#include <stdio.h>
#include <time.h>

#include "example_apps.h"
#include "round_protocol.h"

static void show(const round_runtime_t *runtime)
{
    round_view_t view;
    if (round_render(runtime, &view)) {
        printf("[%s]\n%s\n%s\n\n", view.title, view.body, view.footer);
    }
}

int main(void)
{
    example_state_t state;
    round_app_t apps[EXAMPLE_APP_COUNT];
    round_runtime_t runtime;
    round_parser_t parser = {0};
    example_apps_init(&state, apps);
    if (!round_init(&runtime, apps, EXAMPLE_APP_COUNT)) {
        return 1;
    }
    show(&runtime);
    int byte;
    while ((byte = getchar()) != EOF) {
        struct timespec now;
        if (timespec_get(&now, TIME_UTC) != TIME_UTC) {
            return 1;
        }
        uint32_t milliseconds = (uint32_t)((uint64_t)now.tv_sec * 1000 +
                                         (uint64_t)now.tv_nsec / 1000000);
        round_event_t event;
        if (round_parser_feed(&parser, (uint8_t)byte, milliseconds, &event) &&
            round_dispatch(&runtime, &event)) {
            show(&runtime);
        }
    }
    return ferror(stdin) ? 1 : 0;
}

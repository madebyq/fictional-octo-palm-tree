#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "example_apps.h"
#include "round_protocol.h"

static bool feed(round_parser_t *parser, const char *line, uint32_t now, round_event_t *event)
{
    bool result = false;
    for (size_t i = 0; line[i]; ++i) {
        bool parsed = round_parser_feed(parser, (uint8_t)line[i], now, event);
        assert(!parsed || line[i] == '\n');
        result = parsed || result;
    }
    return result;
}

static void unterminated_render(const void *context, round_view_t *view)
{
    (void)context;
    memset(view, 'x', sizeof(*view));
}

static void test_registry(void)
{
    round_runtime_t runtime;
    round_view_t view;
    round_app_t apps[] = {{.id = "test", .render = unterminated_render}};
    assert(!round_init(NULL, apps, 1));
    assert(!round_init(&runtime, NULL, 1));
    assert(!round_render(&runtime, &view));
    assert(!round_init(&runtime, apps, 0));
    assert(round_init(&runtime, apps, 1));
    assert(round_render(&runtime, &view));
    assert(strlen(view.title) == sizeof(view.title) - 1);
    assert(strlen(view.body) == sizeof(view.body) - 1);
    assert(strlen(view.footer) == sizeof(view.footer) - 1);
    assert(!round_render(&runtime, NULL));
    round_app_t duplicate[] = {apps[0], apps[0]};
    assert(!round_init(&runtime, duplicate, 2));
    assert(runtime.count == 0);
    apps[0].render = NULL;
    assert(!round_init(&runtime, apps, 1));
    apps[0].render = unterminated_render;
    apps[0].id = "";
    assert(!round_init(&runtime, apps, 1));
    apps[0].id = NULL;
    assert(!round_init(&runtime, apps, 1));
}

static void test_apps(void)
{
    example_state_t state;
    round_app_t apps[EXAMPLE_APP_COUNT];
    round_runtime_t runtime;
    round_view_t view;
    example_apps_init(&state, apps);
    assert(round_init(&runtime, apps, EXAMPLE_APP_COUNT));
    assert(round_render(&runtime, &view));
    assert(strcmp(view.title, "Round Foundation") == 0);

    round_event_t event = {.type = ROUND_EVENT_PREVIOUS};
    assert(round_dispatch(&runtime, &event));
    assert(runtime.active == EXAMPLE_APP_COUNT - 1);
    assert(round_render(&runtime, &view));
    assert(strcmp(view.body, "Waiting for telemetry") == 0);
    event.type = ROUND_EVENT_NEXT;
    assert(round_dispatch(&runtime, &event));
    assert(runtime.active == 0);

    event = (round_event_t){.type = ROUND_EVENT_AGENT, .data.agent = ROUND_AGENT_WORKING};
    assert(round_dispatch(&runtime, &event));
    assert(state.agent == ROUND_AGENT_WORKING);
    event = (round_event_t){.type = ROUND_EVENT_SOLAR,
                            .data.solar = {.watts = 1200, .battery_percent = 75}};
    assert(round_dispatch(&runtime, &event));
    assert(state.solar_available && state.solar_watts == 1200 && state.battery_percent == 75);

    event.type = ROUND_EVENT_NEXT;
    assert(round_dispatch(&runtime, &event));
    assert(round_render(&runtime, &view));
    assert(strcmp(view.body, "Working") == 0);
    assert(round_dispatch(&runtime, &event));
    assert(round_render(&runtime, &view));
    assert(strcmp(view.body, "Generation: 1200 W\nBattery: 75%") == 0);
    assert(strstr(view.footer, "not live"));

    event = (round_event_t){.type = ROUND_EVENT_SOLAR, .data.solar.battery_percent = 101};
    assert(!round_dispatch(&runtime, &event));
    assert(state.battery_percent == 75);
    event = (round_event_t){.type = ROUND_EVENT_AGENT, .data.agent = (round_agent_state_t)99};
    assert(!round_dispatch(&runtime, &event));
    assert(state.agent == ROUND_AGENT_WORKING);
    event.type = (round_event_type_t)99;
    assert(!round_dispatch(&runtime, &event));
    assert(!round_dispatch(NULL, &event));
    assert(!round_dispatch(&runtime, NULL));
}

static void test_protocol(void)
{
    round_parser_t parser = {0};
    round_event_t event;
    const char *const states[] = {
        "!idle\n", "!working\r\n", "!attention\n", "!complete\n", "!surprise\n"
    };
    for (size_t i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
        assert(feed(&parser, states[i], 100, &event));
        assert(event.type == ROUND_EVENT_AGENT);
        assert(event.data.agent == (round_agent_state_t)i);
    }
    assert(feed(&parser, "/app next\n", 100, &event));
    assert(event.type == ROUND_EVENT_NEXT);
    assert(feed(&parser, "/app previous\n", 100, &event));
    assert(event.type == ROUND_EVENT_PREVIOUS);
    assert(feed(&parser, "@solar 4294967295 100\n", 100, &event));
    assert(event.type == ROUND_EVENT_SOLAR);
    assert(event.data.solar.watts == UINT32_MAX && event.data.solar.battery_percent == 100);
    assert(feed(&parser, "@solar 0 0\n", 100, &event));
    assert(event.data.solar.watts == 0 && event.data.solar.battery_percent == 0);

    const char *const invalid[] = {
        "\n", "\r\n", "!WORKING\n", "!idle extra\n", "!idle\rjunk\n", "!idle\r\r\n",
        "@solar -1 40\n", "@solar 4294967296 40\n", "@solar 1 101\n",
        "@solar 1 -1\n", "@solar  1 2\n", "@solar 1 2tail\n", "@solar 1 2 \n",
        "@solar 1\n", "@solar 1 \n", "/app unknown\n", "\t!idle\n"
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        event = (round_event_t){.type = ROUND_EVENT_PREVIOUS};
        assert(!feed(&parser, invalid[i], 100, &event));
        assert(event.type == ROUND_EVENT_PREVIOUS);
        assert(feed(&parser, "!idle\n", 100, &event));
    }
    assert(!round_parser_feed(NULL, 'x', 0, &event));
    assert(!round_parser_feed(&parser, 'x', 0, NULL));
}

static void test_parser_recovery(void)
{
    round_parser_t parser = {0};
    round_event_t event;
    for (size_t i = 0; i < ROUND_COMMAND_CAPACITY + 10; ++i) {
        assert(!round_parser_feed(&parser, 'x', 0, &event));
    }
    assert(!feed(&parser, "!idle\n", 0, &event));
    assert(feed(&parser, "!working\n", 0, &event));
    assert(!round_parser_feed(&parser, 0, 0, &event));
    assert(!feed(&parser, "!idle\n", 0, &event));
    assert(feed(&parser, "!idle\n", 0, &event));
    assert(!round_parser_feed(&parser, 0xff, 0, &event));
    assert(!feed(&parser, "!idle\n", 0, &event));

    assert(!feed(&parser, "!wor", 10, &event));
    assert(!feed(&parser, "king\n", 1010, &event));
    assert(feed(&parser, "!idle\n", 1011, &event));
    assert(!feed(&parser, "!idle", UINT32_MAX - 100, &event));
    assert(round_parser_feed(&parser, '\n', 50, &event));
    assert(!feed(&parser, "!idle", UINT32_MAX - 100, &event));
    assert(!round_parser_feed(&parser, '\n', 1000, &event));
    assert(feed(&parser, "!idle\n", 1001, &event));
}

int main(void)
{
    test_registry();
    test_apps();
    test_protocol();
    test_parser_recovery();
    puts("All round foundation checks passed");
    return 0;
}

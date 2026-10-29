#include "round_protocol.h"

#include <string.h>

static bool parse_uint(const char **cursor, uint32_t limit, uint32_t *value)
{
    const char *p = *cursor;
    uint32_t result = 0;
    if (*p < '0' || *p > '9') {
        return false;
    }
    while (*p >= '0' && *p <= '9') {
        uint32_t digit = (uint32_t)(*p++ - '0');
        if (result > limit / 10 || (result == limit / 10 && digit > limit % 10)) {
            return false;
        }
        result = result * 10 + digit;
    }
    *cursor = p;
    *value = result;
    return true;
}

static bool parse_line(const char *line, round_event_t *event)
{
    static const char *const states[] = {
        "!idle", "!working", "!attention", "!complete", "!surprise"
    };
    for (size_t i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
        if (strcmp(line, states[i]) == 0) {
            *event = (round_event_t){
                .type = ROUND_EVENT_AGENT, .data.agent = (round_agent_state_t)i
            };
            return true;
        }
    }
    if (strcmp(line, "/app next") == 0 || strcmp(line, "/app previous") == 0) {
        *event = (round_event_t){
            .type = strcmp(line, "/app next") == 0 ? ROUND_EVENT_NEXT : ROUND_EVENT_PREVIOUS
        };
        return true;
    }
    if (strncmp(line, "@solar ", 7) == 0) {
        const char *cursor = line + 7;
        uint32_t watts, battery;
        if (!parse_uint(&cursor, UINT32_MAX, &watts) || *cursor != ' ') {
            return false;
        }
        ++cursor;
        if (!parse_uint(&cursor, 100, &battery) || *cursor != '\0') {
            return false;
        }
        *event = (round_event_t){
            .type = ROUND_EVENT_SOLAR,
            .data.solar = {.watts = watts, .battery_percent = (uint8_t)battery}
        };
        return true;
    }
    return false;
}

bool round_parser_feed(round_parser_t *parser, uint8_t byte, uint32_t now_ms,
                       round_event_t *event)
{
    if (!parser || !event) {
        return false;
    }
    if (parser->length && (uint32_t)(now_ms - parser->started_ms) >= ROUND_COMMAND_TIMEOUT_MS) {
        parser->discarding = true;
    }
    if (byte == '\n') {
        bool valid = false;
        if (!parser->discarding) {
            if (parser->length && parser->line[parser->length - 1] == '\r') {
                --parser->length;
            }
            parser->line[parser->length] = '\0';
            valid = parse_line(parser->line, event);
        }
        *parser = (round_parser_t){0};
        return valid;
    }
    if (parser->discarding) {
        return false;
    }
    if ((byte < 32 && byte != '\r') || byte > 126 ||
        (parser->length && parser->line[parser->length - 1] == '\r') ||
        parser->length == sizeof(parser->line) - 1) {
        parser->discarding = true;
        return false;
    }
    if (!parser->length) {
        parser->started_ms = now_ms;
    }
    parser->line[parser->length++] = (char)byte;
    return false;
}

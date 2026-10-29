#ifndef ROUND_PROTOCOL_H
#define ROUND_PROTOCOL_H

#include "round_core.h"

#define ROUND_COMMAND_CAPACITY 64
#define ROUND_COMMAND_TIMEOUT_MS 1000U

typedef struct {
    char line[ROUND_COMMAND_CAPACITY];
    size_t length;
    uint32_t started_ms;
    bool discarding;
} round_parser_t;

/* Zero-initialize before use. Returns true only for a complete, valid event.
 * Malformed, expired and oversized lines are discarded through the next LF. */
bool round_parser_feed(round_parser_t *parser, uint8_t byte, uint32_t now_ms,
                       round_event_t *event);

#endif

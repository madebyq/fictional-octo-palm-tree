---
name: round-app-development
description: Add or extend apps, typed events, views, and serial commands in the round-device framework. Use for agent companion, solar dashboard, or new application features.
---

# Round application development

## Establish the change boundary

Read `README.md`, `components/round_core/include/round_core.h`,
`components/round_core/round_core.c`, `main/example_apps.h`, and
`main/example_apps.c` from the repository root.
Inspect `tests/test_round.c` for the established assertion style.
Decide whether the request needs app state, a new event type, or a transport;
do not add network or hardware dependencies to implement a pure app feature.

## Implement the application

1. Define state and a descriptor with a unique, nonempty ID, a render callback,
   and an optional event callback. Ensure descriptors and contexts outlive the
   runtime; initialize state explicitly.
2. Register the descriptor. For the existing examples, keep `EXAMPLE_APP_COUNT`
   and the array initialization in sync. Update Home's overview if appropriate.
3. Keep callbacks platform-independent and nonblocking. Write bounded strings
   to `round_view_t`; do not call LVGL, FreeRTOS, or network APIs there.
4. For a new data model, add a typed `round_event_t` payload and validate it in
   `round_dispatch` before any callback sees it. Keep payloads safe to copy by
   value through the firmware queue; avoid borrowed pointers.
5. Preserve broadcasts to inactive apps. Put navigation in the runtime, not in
   individual app callbacks.
6. Only extend serial parsing if the feature needs a serial contract. Read
   `components/round_core/round_protocol.c` and its header first. Preserve line
   bounds, overflow checks, invalid-input rejection, and discard-through-LF
   recovery. Keep parsing separate from application mutation.

## Prove behavior

- Extend `tests/test_round.c` with valid state updates, invalid values without
  mutation, initial state, rendering, and updates received while the app is hidden.
- For parser changes, test LF/CRLF, maximum values, numeric overflow, malformed
  and oversized lines, timeout/clock wraparound, and recovery on the next line.
- Use the `round-validation` skill to build, test, and exercise the simulator.
  If new source files are introduced, update both host and firmware CMake source
  lists as needed.
- Update README app descriptions and serial contracts. Label mock, unavailable,
  stale, and last-received data honestly; do not claim unverified backend compatibility.

Finish by summarizing behavior changed, checks actually run, and outstanding
firmware/device verification.

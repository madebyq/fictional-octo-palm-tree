---
name: round-hardware-integration
description: Integrate or change Waveshare ESP32-S3 base/-B BSP, LVGL display, capacitive touch, or firmware transport adapters. Use for device-specific work rather than portable application logic.
---

# Round hardware integration

## Confirm hardware and APIs

1. Read `README.md`, `main/app_main.c`, `main/CMakeLists.txt`,
   `main/idf_component.yml`, and `sdkconfig.defaults`.
2. Confirm the exact board variant before changing BSP or pin assumptions.
   The supported base/-B has CO5300 QSPI display and CST9217 I²C touch.
   The 1.75C is not interchangeable. Ask for the label if the variant is unclear.
3. Check the installed/pinned BSP and LVGL headers for actual API signatures,
   return conventions, and lock behavior. Do not infer signatures from similarly
   named APIs for other boards or versions.
4. Use the vendor sources linked in README. Document new sources and distinguish
   source declarations from successfully resolved, compiled, or tested dependencies.

## Preserve integration boundaries

- Keep hardware calls in firmware adapters and the shell, not `round_core` or
  app callbacks. Reuse the vendor BSP instead of duplicating panel setup.
- Keep one owner task for app dispatch and rendering. Transport tasks validate
  input and enqueue copied events; they must not mutate app contexts or LVGL.
- Hold the display lock for LVGL calls outside its callbacks and release it on
  every exit path. Never wait on a full event queue from a touch callback.
- Check initialization and task/queue creation failures. Bound producer waits,
  queue growth, input sizes, and network operations; make dropped events observable
  without exposing credentials.
- Keep content within the circular display's visible area. Review brightness,
  touch orientation, clipping, and inactivity behavior for the actual panel.
- For a requested network adapter, verify the backend contract and require
  authenticated TLS, bounded payloads/timeouts, reconnect/backoff, and explicit
  freshness semantics. Do not enable unauthenticated listeners, plaintext OTA,
  or put cloud API keys in firmware.

## Validate in layers

Use `round-validation` for portable checks and a firmware build with the intended
ESP-IDF environment. Host tests do not compile `main/app_main.c`.

Only with a confirmed board/port and user approval, flash the device. Check boot
logs, display initialization, touch navigation, USB input, updated views,
queue-pressure behavior, and reconnect/failure handling for changed adapters.
Do not flash as a substitute for a host or firmware build check.

Record which dependencies resolved, whether firmware compiled, and which physical
checks were performed. If tools, registry access, or hardware are unavailable,
report that explicitly rather than declaring device support verified.

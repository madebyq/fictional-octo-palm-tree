# Repository context

This is a C11/ESP-IDF application foundation for the Waveshare
ESP32-S3-Touch-AMOLED-1.75 / -B (case variant), with a 466 × 466 round display.
The 1.75C is a different board; do not substitute its BSP or pin configuration.
Read `README.md` for current support limitations and integration contracts.

## Code map and invariants

- `components/round_core/`: portable app registration, event dispatch, views,
  and bounded serial parsing. Keep it independent of ESP-IDF, FreeRTOS, and LVGL.
- `main/example_apps.*`: application state and callbacks. Descriptors and
  contexts must outlive the runtime; IDs must be unique and nonempty.
- `main/app_main.c`: BSP, LVGL shell, USB input, and event queue integration.
  One owner task dispatches events and renders. Producers enqueue copied
  `round_event_t` values; do not mutate application state from producer tasks.
- Non-navigation events update all apps, including inactive apps. Navigation
  belongs to the runtime. Keep callbacks bounded and nonblocking.
- Keep core storage bounded and allocation-free. Use size-aware writes to view
  buffers and validate data before dispatch. Invalid input must not change state.
- Hold the BSP display lock for LVGL calls outside LVGL callbacks. Do not block
  on queue sends from touch callbacks while they hold the display lock.
- `host/simulator.c` is a terminal adapter, not a display or hardware emulator.
- `tests/test_round.c` uses C assertions and CTest; extend this infrastructure
  rather than introducing another test framework.

## Build and verification

Run commands from the repository root, using an out-of-tree build directory:

```sh
cmake -S . -B /tmp/round-build -DROUND_HOST=ON
cmake --build /tmp/round-build
ctest --test-dir /tmp/round-build --output-on-failure
printf '!working\n/app next\n@solar 1200 75\n/app next\n' | /tmp/round-build/round_simulator
```

For firmware changes, activate ESP-IDF 5.5.x and run `idf.py build`.
The target is ESP32-S3; dependencies are declared in `main/idf_component.yml`
and defaults in `sdkconfig.defaults`. Never equate host success with firmware
or physical-device verification. Report missing tools and unavailable hardware.
Flashing requires an explicitly selected connected device and user approval.

## Change boundaries and security

- Make focused changes and preserve existing behavior unless a change is requested.
- Keep serial parsing bounded, with LF/CRLF handling, timeout/overflow recovery,
  numeric bounds, and tests for malformed inputs.
- Do not describe the solar example as a verified solar-agent connector or the
  agent example as a full Agent Buddy implementation.
- Do not invent backend contracts. Verify sources and document compatibility limits.
- No network service is enabled by default. New connectors must authenticate
  peers, use TLS for network credentials/data, bound payloads and timeouts, and
  keep cloud API keys on the backend. Do not commit credentials or log secrets.
- Preserve the distinction between last received samples and live/fresh telemetry.
- Do not commit build output, managed components, or local `sdkconfig`. Retain
  `dependencies.lock` after a successful firmware dependency resolution.
- Update README behavior, setup, and source links when related code changes.
- Keep Copilot customization to instructions and skills; do not add custom
  agent definitions or prompt files.

## Task-specific skills

Use the relevant workflow in `.github/skills/`:

- `round-app-development`: add or extend application state, events, and views.
- `round-hardware-integration`: change BSP, LVGL, touch, or device adapters.
- `round-validation`: verify host behavior and distinguish firmware/hardware checks.

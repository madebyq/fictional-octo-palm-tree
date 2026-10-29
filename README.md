# Round application foundation

A small ESP-IDF framework for the **Waveshare ESP32-S3-Touch-AMOLED-1.75 / -B
(with case)**: 466 × 466 round AMOLED, capacitive touch, ESP32-S3, 16 MB flash
and 8 MB PSRAM. It provides an allocation-free app/event core, an LVGL screen
shell, touch navigation, and a bounded USB serial command adapter.

**Validation status:** the host build and tests pass. ESP-IDF is not installed in
the implementation environment, so the firmware has not been compiled, flashed,
or verified on a physical board. It integrates the vendor BSP. The host simulator exercises the same
core, examples, and parser, not the display or touch drivers. The **1.75C is a
different board and is not supported by this configuration**; check the label
before flashing. The base/-B uses CO5300 QSPI and CST9217 I²C controllers.

## What is included

- **Home:** application launcher overview; tap **Next** to cycle through apps.
- **Agent companion:** idle, working, attention, complete, and surprise states.
- **Solar dashboard:** generation and battery telemetry. Starts empty, and labels
  received data as the last sample, never as a live connection or fabricated data.
- Fixed-size events, view buffers, and command input; no heap allocation in the core.
- A single firmware owner task for app state. Serial and touch producers send
  copied events through a bounded FreeRTOS queue; all LVGL access is locked.

Wi-Fi and BLE are hardware capabilities, **not enabled services in this starter**.
It needs no cloud credentials and opens no network listener. AI execution,
voice/audio, provisioning, persistence, animations, OTA, and real solar backend
connectors are intentionally left to applications.

## Build and flash

Install [ESP-IDF 5.5.x](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/)
with ESP32-S3 tools and activate its environment (`. /path/to/esp-idf/export.sh`).
From the repository root:

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Use your board's native USB Serial/JTAG port; replace the port for your OS.
If necessary, hold BOOT while resetting to enter download mode. `sdkconfig.defaults`
selects the base/-B's 16 MB flash, octal PSRAM, and USB Serial/JTAG console.
The manifest requests BSP **3.0.1** and LVGL **9.4.0**, plus their transitive
drivers, through the Component Manager. The BSP version is taken from the vendor
source manifest; the component registry was unreachable during implementation,
so publication/resolution of these exact versions remains unverified.
ESP-IDF must be >=5.5 and <6.0.
After a successful firmware build, retain the generated `dependencies.lock` for
reproducible transitive versions; generated sources and local `sdkconfig` are ignored.

The display runs at 30% brightness with content placed in the central area of the
circle. Verify touch orientation, clipping, USB input, and panel initialization
on the actual device. For long-running products, add inactivity dimming/sleep and
burn-in mitigation. This is a development foundation, not production firmware.

## Run without hardware

Only a C11 compiler and CMake are needed. No ESP-IDF, LVGL, or downloaded packages
are used by the host build:

```sh
cmake -S . -B /tmp/round-build -DROUND_HOST=ON
cmake --build /tmp/round-build
ctest --test-dir /tmp/round-build --output-on-failure
printf '!working\n/app next\n@solar 1200 75\n/app next\n' | /tmp/round-build/round_simulator
```

The simulator prints Home, then the agent's **Working** state and the solar
sample. Tests cover registration, navigation/wraparound, inactive-app updates,
buffer termination, invalid events, numeric overflow, CRLF, timeout/clock
wraparound, and recovery after oversized or malformed commands.

## Serial integration contract

Send ASCII lines over USB serial (115200 baud where required by the terminal).
LF and CRLF are accepted. Commands are case-sensitive:

| Line | Effect |
| --- | --- |
| `!idle` / `!working` / `!attention` / `!complete` / `!surprise` | Update the agent app, even when hidden |
| `/app next` / `/app previous` | Navigate with wraparound |
| `@solar 1200 75` | Update generation in watts and battery percent |

`@solar` accepts two unsigned decimal integers: watts 0–4294967295 and battery
0–100, with one separating space and no trailing characters. It is **this
starter's local example contract**, not a verified solar-agent API.
Lines may contain at most 63 bytes before LF (including an optional CR) and must
complete within 1000 ms from their first byte. Invalid/control/non-ASCII input,
overflow, and expired partial lines are dropped through the next LF, so send a
newline before retrying an interrupted command. Invalid lines do not change app
state. The firmware logs queue-full drops; there is no delivery acknowledgment.
Reboot resets all state. Do not send secrets: USB serial is not authenticated.

The five `!state` commands are a **subset** of Agent Buddy's serial protocol.
This project does not implement its discovery, handshake, pairing, icon, usage,
firmware-update, or HTTP protocols, and is **not a drop-in Agent Buddy device**.
Use a bridge that writes these lines rather than assuming daemon compatibility.

## Build your own application

1. Define an application context and `round_app_t` descriptor with a unique ID.
   Implement `render(context, view)` and optionally `on_event(context, event)`.
   See `main/example_apps.c`; no ESP-IDF or LVGL calls belong in these callbacks.
2. Register descriptors in a stable array passed to `round_init`. Descriptors
   and contexts must outlive the runtime. Increase the example array/count or
   replace it with your own registry.
3. Add typed events to `round_core.h` and validation in `round_dispatch` when
   introducing a new data model. Events reach every app, including hidden apps;
   navigation is handled by the runtime. Keep callbacks bounded and nonblocking.
4. Put hardware/transport work in adapters, not the core. A Wi-Fi connector
   should validate backend data, then enqueue copied events rather than touching
   app state or LVGL from its task. Extend `app_main.c` to start that adapter.
   Keep API keys on a trusted backend, require authenticated TLS, bound payload
   sizes/timeouts, and implement reconnect/backoff and data freshness before
   describing readings as live. Do not copy plain-HTTP OTA into a product.
5. Add host checks in `tests/test_round.c`, then build and test on the board.

`components/round_core` owns app dispatch and serial parsing; `main/example_apps.*`
owns sample app state; `main/app_main.c` owns the BSP/LVGL/FreeRTOS integration.
`host/simulator.c` is a terminal adapter, not an emulator. Swap the BSP/shell to
port the framework without rewriting the app callbacks.

## GitHub Copilot usage

[Repository instructions](.github/copilot-instructions.md) give Copilot the code
map, state-ownership rules, build commands, and security boundaries.
Task-specific workflows are available as repository skills:

| Skill | Use it for |
| --- | --- |
| [round-app-development](.github/skills/round-app-development/SKILL.md) | Adding apps, events, views, or serial commands |
| [round-hardware-integration](.github/skills/round-hardware-integration/SKILL.md) | BSP, LVGL, touch, and firmware adapters |
| [round-validation](.github/skills/round-validation/SKILL.md) | Host tests, simulator checks, and firmware/hardware validation scope |

Open the repository in a supported Copilot client with repository custom
instructions enabled. Describe the task in chat; Copilot can discover and load
skills by their descriptions, or you can explicitly ask it to use a named skill.
These are instructions and reusable workflows, not executable automation:
they do not install ESP-IDF, enable cloud services, or verify hardware.
No custom agent definitions or prompt files are included.

Client support and settings vary; see GitHub's
[repository instructions guide](https://docs.github.com/en/copilot/how-tos/configure-custom-instructions/add-repository-instructions)
and [agent skills guide](https://docs.github.com/en/copilot/how-tos/copilot-on-github/customize-copilot/customize-cloud-agent/add-skills)
for discovery and setup.

## Sources and inspiration

- [Waveshare board and case variants](https://www.waveshare.com/esp32-s3-touch-amoled-1.75.htm)
  and [vendor examples/hardware reference](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75).
- [Vendor BSP source and dependency manifest](https://github.com/waveshareteam/Waveshare-ESP32-components/tree/main/bsp/esp32_s3_touch_amoled_1_75):
  display/touch bring-up is delegated to this managed component, not recreated here.
- [Dan Wahlin's Agent Buddy](https://github.com/DanWahlin/agent-buddy):
  inspiration for separating companion state, board drivers, and bounded serial
  input. Its Arduino implementation is not used as an ESP-IDF dependency; this
  project's application code is independently implemented.
- [madebyq/solar-agent](https://github.com/madebyq/solar-agent): returned 404 during
  implementation. Its source, license, and API could not be verified; the solar
  example is an integration placeholder, not a claim of compatibility.
---
name: round-validation
description: Build, test, and manually exercise the round framework with CMake, CTest, and the terminal simulator, then scope ESP-IDF and hardware verification. Use to validate changes or investigate local regressions.
---

# Round framework validation

## Select checks

Read root `CMakeLists.txt`, `tests/test_round.c`, and the README build sections.
Use existing tools; do not add test frameworks or install firmware dependencies
just to run the portable checks. For documentation-only changes, review paths,
commands, links, and skill metadata; no firmware build is necessary.

## Host build and tests

Run from the repository root with a dedicated temporary build directory:

```sh
cmake -S . -B /tmp/round-build -DROUND_HOST=ON
cmake --build /tmp/round-build
ctest --test-dir /tmp/round-build --output-on-failure
```

The host build uses C11, strict warnings on the core/tests, and no downloaded
components. Investigate failures rather than disabling assertions or weakening
existing tests. Add focused regression cases to `tests/test_round.c`.

Exercise the shared parser and example apps:

```sh
printf '!working\n/app next\n@solar 1200 75\n/app next\n' | /tmp/round-build/round_simulator
```

Verify Home appears first, the agent view reports Working after navigation,
and the solar view shows 1200 W / 75% as the last sample, not a live connection.
For changed commands, also exercise invalid input followed by a valid line and
check that parsing recovers without unintended state changes.

When investigating memory or integer issues, use existing compiler sanitizers
if available in a separate build:

```sh
cmake -S . -B /tmp/round-sanitize -DROUND_HOST=ON -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build /tmp/round-sanitize
ctest --test-dir /tmp/round-sanitize --output-on-failure
```

## Firmware and hardware checks

- Host checks cover the core, parser, and example apps, not BSP/LVGL/FreeRTOS.
- For firmware-affecting changes, activate the README's ESP-IDF environment and
  run `idf.py build`. Review dependency resolution and target/configuration.
- If ESP-IDF or registry access is unavailable, state that firmware is unverified.
- Use `round-hardware-integration` for approved physical-device validation.
  Neither host tests nor a firmware build prove display/touch behavior.

## Completion evidence

Review the diff for unintended files, credentials, generated builds, and managed
components. Keep API secrets out of code, logs, screenshots, and fixtures.
Report exact checks and results separately for host, firmware, and hardware;
include skipped checks and reasons. Keep README validation claims consistent
with observed results rather than replacing unknowns with assumptions.

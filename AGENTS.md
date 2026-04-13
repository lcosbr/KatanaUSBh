# AGENTS.md

Agentic coding instructions for KatanaUSBH - ESP32-S3 Katana foot controller firmware.

## Build Commands

### PlatformIO Workflow
```sh
pio run -e esp32-s3-devkitc-1              # Build
pio run -e esp32-s3-devkitc-1 -t upload    # Flash to device
pio device monitor -b 115200              # Serial output (115200 baud)
```

### Notes
- `platformio.ini` sets `[platformio] core_dir = c:/dev/pio`
- Single environment: `esp32-s3-devkitc-1` with `framework = arduino`
- No separate test framework exists; `test/` contains stock PlatformIO placeholders
- Default verification is a focused PlatformIO build

## Code Layout

- `src/main.cpp` - App entrypoint: USB connect/disconnect, startup scanning, delayed preset changes, background resync
- `src/KatanaLogic.h` - Katana SysEx protocol, preset/effect cache, MIDI parsing (header-only)
- `src/oledControl.h` - SSD1306 button labels (Wire) + SH1107 main display (Wire1) (header-only)
- `src/buttonController.h` - Button scanning, debounce, LED control (header-only)
- `src/KatanaDefines.h` - Katana protocol addresses and SysEx constants
- `src/ButtonConfig.h` - Button and LED pin mappings
- `src/OledConfig.h` - I2C pins and display configuration
- `src/CMakeLists.txt` - Recursively globs `src/*.*`; new code goes under `src/`

## Code Style Guidelines

### Imports and Dependencies
- Use angle brackets for library includes: `#include <Arduino.h>`, `#include <usbh_midi.h>`
- Use double quotes for local headers: `#include "KatanaLogic.h"`, `#include "ButtonConfig.h"`
- Always include `<Arduino.h>` first in header files
- Follow order: 1) Standard libs, 2) Third-party libs, 3) Local headers

### Naming Conventions
- Classes: `PascalCase` (e.g., `KatanaController`, `Adafruit_SSD1306`)
- Functions: `camelCase` (e.g., `scanButtons()`, `setLed()`)
- Constants/enums: `SCREAMING_SNAKE_CASE` with `const` or `#define` (e.g., `SYSEX_WRITE`, `LED_0_PIN`)
- Global variables: `camelCase` with descriptive names (e.g., `connected`, `currentPreset`)
- Member variables in classes: `camelCase` (e.g., `midi`, `onEffectChange`)
- Typedefs for function pointers: `camelCase` with `Callback` suffix (e.g., `StateCallback`, `NameCallback`)
- Arrays/pins: `PascalCase` for pin arrays (e.g., `LED_PINS[8]`, `BTN_PINS[10]`)

### Types
- Use fixed-width types from `<stdint.h>` for hardware: `uint8_t`, `uint16_t`, `uint32_t`
- Use `int` for simple loop counters and time values
- Use `bool` for state flags (`true`/`false`, not `1`/`0`)
- Use `String` from Arduino for string handling (not `std::string`)
- Prefer `constexpr` for compile-time constants over `#define` when possible

### Formatting
- Indentation: 2 spaces (no tabs)
- Opening brace on same line: `void function() {`
- Maximum line length: ~100 characters (use common sense)
- Use blank lines to separate logical sections in functions
- Comment style: `// Comment` for single lines, `// Section:` for headers

### Error Handling
- Check return values from hardware init functions
- Guard bounds on array access (e.g., `if (index < 8)` before `LED_PINS[index]`)
- Use early returns to reduce nesting: `if (!condition) return false;`
- No exceptions; all error handling is through return codes or bools

### Patterns
- Header guards: `#ifndef NAME_H` / `#define NAME_H` / `#endif`
- Class member initialization in constructor: `ClassName(args) : member(args) { }`
- Use `memset()` to zero arrays, `strcpy()` / `sprintf()` for string building
- Debounce pattern: store `lastStableBtn[]`, `lastChangeMs[]`, use `millis()` for timing

## Repo-Specific Quirks

### USB Host/MIDI
- Uses `USB-Host-Shield-20`: `#include <usbhub.h>`, `#include <usbh_midi.h>`
- NOT ESP-IDF/TinyUSB host APIs
- Preserve these build flags in `platformio.ini` unless explicitly changing USB behavior:
  ```
  -D ARDUINO_USB_MODE=0
  -D ARDUINO_USB_CDC_ON_BOOT=0
  ```

### Startup Behavior
- `main.cpp` does significant blocking work on first device connect:
  1. Enables editor mode (twice)
  2. Scans all 8 presets to populate cache
  3. Reloads preset 1
  4. Forces effect index 3 (MUTE) active
- Keep this in mind before changing startup timing or initial UI state

### SDK Config
- `platformio.ini` points `board_build.sdkconfig` at `sdkconfig.defaults`
- Repo root contains auto-generated `sdkconfig.esp32-s3-devkitc-1`
- Verify the intended SDK config source before making sdkconfig changes

## Generated And Vendored Files

- `.pio/` - Build output plus downloaded library deps; do not hand-edit
- `.vscode/launch.json`, `.vscode/c_cpp_properties.json` - Auto-generated
- `managed_components/` - Third-party code pinned by `dependencies.lock`
- Stock PlatformIO directories: `include/`, `lib/`, `test/` (placeholders)

## Common Tasks

### Add a New Effect Button
1. Add button LED pin to `src/ButtonConfig.h` (LED_X_PIN)
2. Add button input pin to `src/ButtonConfig.h` (BTTN_X_PIN)
3. Add effect label to `BTN_LABELS[]` in `src/oledControl.h`
4. Add effect address to `src/KatanaDefines.h` if new effect
5. Add to `effectsAddr[]` array in `src/KatanaLogic.h` at correct index
6. Handle in `toggleEffect()` and `loadFromCache()` in `KatanaLogic.h`

### Modify Startup Sequence
- Edit `scanAllPresetsOnStartup()` in `src/main.cpp`
- Adjust `waitForPatchLoad()` delays as needed
- Remember blocking delays affect USB enumeration timing

### Debug Serial Output
- Serial is initialized at 115200 baud in `setup()`
- Add `Serial.println()` calls for troubleshooting
- Check serial monitor with `pio device monitor -b 115200`

## Hardware Notes

- Board: ESP32-S3-DevKitC-1
- Flash: 16MB (configured in platformio.ini)
- Button displays: SSD1306 128x32 on TCA9548A multiplexer (Wire, I2C)
- Main display: SH1107 128x128 (Wire1, I2C)
- USB Host: MAX3421E via USB-Host-Shield-20 library
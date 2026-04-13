# Open Points

Static review of the current firmware codebase.

## Confirmed Decisions

- The delayed preset send is intentional debounce behavior.
- `SOLO` should stay synchronized with the amp.
- `MUTE` uses standard MIDI CC#7 (Channel Volume): sending `0xB0, 7, 0` mutes (volume 0), `0xB0, 7, 100` unmutes. It is not persistent — MUTE resets to `false` on every preset change, matching the amp's behavior of resetting foot-volume on preset load. MUTE state is never stored in the preset cache.

## Findings

### First Review (9 findings) -- All Fixed

1. ~~Effect toggles can target the wrong amp preset during the preset debounce window.~~
   - **Fixed**: effect toggles are now blocked while `pendingPreset != -1` (`src/main.cpp`).

2. ~~`SOLO` and `MUTE` are not synchronized from the amp.~~
   - **Fixed**: `requestEffectByIndex()` no longer skips indices 0 and 3. `loadFromCache()` loads all 8 indices from the cache without forcing any to `false`.

3. ~~The SysEx parser reads fixed offsets without validating message length first.~~
   - **Fixed**: `processCompleteSysEx()` now returns early if `len < 15` (`src/KatanaLogic.h`).

4. ~~Delayed timers are not `millis()` rollover-safe.~~
   - **Fixed**: timers now store start time and compare elapsed duration with subtraction (`src/main.cpp`).

5. ~~Every SysEx write allocates and frees heap memory.~~
   - **Fixed**: `sendSysExChange()` uses a stack buffer instead of `new[]`/`delete[]` (`src/KatanaLogic.h`).

6. ~~Button OLED redraws reinitialize the SSD1306 driver each time.~~
   - **Fixed**: `showButtonLabel()` no longer calls `displayMux.begin()` (`src/oledControl.h`).

7. ~~Startup scanning does extra waits for effect indices that are never requested.~~
   - **Fixed by #2**: `requestEffectByIndex()` now requests all 8 indices.

8. ~~Callback pointers are not initialized before first use.~~
   - **Fixed**: `onEffectChange` and `onNameChange` initialized to `nullptr` in constructor (`src/KatanaLogic.h`).

9. ~~`src/defines.h` appears to be stale legacy code.~~
   - **Fixed**: file deleted.

### Second Review (16 findings) -- All Fixed

**Correctness:**

- **C1**: ~~`uint8_t << 24` is UB (promotes to signed `int`, shifts into sign bit).~~
  - **Fixed**: address reconstruction in `processCompleteSysEx()` now casts to `(uint32_t)` before shifting (`src/KatanaLogic.h:199-203`).

- **C2**: ~~`sendSysExChange()` has no length guard; large `len` overflows `msg[64]` and `chkBuf`.~~
  - **Fixed**: added `if (len > 48) return;` guard, enlarged `chkBuf` to `[52]` (`src/KatanaLogic.h:79`).

- **C3**: ~~`toggleEffect()` MUTE path uses hardcoded `EN_TUNER` instead of `effectsAddr[index]`.~~
  - **Fixed**: now uses `effectsAddr[index]` (`src/KatanaLogic.h:133`).

- **C5**: ~~Background sync fires 8 effect requests simultaneously, potentially overloading the amp.~~
  - **Fixed**: non-blocking staggered sync -- one request per 20ms via `syncEffectIdx`/`syncStepTime` state machine (`src/main.cpp:186-199`).

**Performance:**

- **P1+P3**: ~~`getCachedName()` returned `String`, causing heap allocation on every call.~~
  - **Fixed**: now returns `const char*` (`src/KatanaLogic.h:186`).

- **P2**: ~~`printCenteredWordWrap()` used `String::substring()` per line.~~
  - **Fixed**: rewritten to take `const char*`, uses stack `char lineBuf[32]` (`src/oledControl.h:74`).

- **P4**: ~~`updateMainDisplay()` built `"P:" + String(presetNum)` on every call.~~
  - **Fixed**: uses `sprintf(pLabel, "P:%d", presetNum)` with stack buffer (`src/oledControl.h:142`).

- **P5**: ~~`effectsAddr[]` was mutable despite being constant data.~~
  - **Fixed**: declared `const uint32_t effectsAddr[8]` (`src/KatanaLogic.h:25`).

- **P8**: ~~`displayMux.setRotation(2)` called on every button label redraw.~~
  - **Fixed**: moved into `initOledOnChannel()` (called once per display) (`src/oledControl.h:40`).

**Robustness:**

- **R3**: ~~`Usb.Init()` failure silently enters infinite loop with no feedback.~~
  - **Fixed**: prints `"USB Host init failed!"` to Serial and shows `"USB FAIL"` on main display before halting (`src/main.cpp:100-104`).

- **R5**: ~~`pendingPreset`, `syncPending`, `syncEffectIdx` not reset on USB disconnect.~~
  - **Fixed**: all three reset in the disconnect handler (`src/main.cpp:132-134`).

- **R6**: ~~`board_build.sdkconfig = sdkconfig.defaults` references non-existent file.~~
  - **Fixed**: changed to `sdkconfig.esp32-s3-devkitc-1` which is the actual file (`platformio.ini:36`).

**Style/Cleanup:**

- **S1**: ~~`EN_TUNER` name is misleading for a MUTE-via-foot-volume address.~~
  - **Fixed**: renamed to `EN_MUTE` across `KatanaDefines.h` and `KatanaLogic.h`.

- **S2**: ~~`PANNEL_NAME` typo.~~
  - **Fixed**: renamed to `PANEL_NAME` across `KatanaDefines.h` and `KatanaLogic.h`.

- **S4**: ~~`onSysExNameChange` callback used `String` parameter.~~
  - **Fixed**: `NameCallback` typedef changed to `const char*`; name parsing in `processCompleteSysEx` no longer creates `String` (`src/KatanaLogic.h`).

- **S5**: ~~Dead `requestVolume()` method and unused `ADDR_VOLUME` constant.~~
  - **Fixed**: both removed from `KatanaLogic.h` and `KatanaDefines.h`.

- **S6**: ~~`oledControl.h` and `buttonController.h` lack include guards.~~
  - **Fixed**: added `#ifndef`/`#define`/`#endif` guards to both files.

### Third Review (13 findings) -- All Fixed

**Correctness:**

- **C1**: ~~`parseIncoming()` silently drops overflow bytes but still processes the truncated SysEx.~~
  - **Fixed**: added `sysExOverflow` flag; when buffer exceeds 128 bytes the flag is set, and `processCompleteSysEx` is skipped when `SYSEX_END` arrives (`src/KatanaLogic.h`).

- **C2**: ~~`processCompleteSysEx()` does not validate the Roland SysEx header before parsing.~~
  - **Fixed**: added `memcmp(&msg[1], SYSEX_HEADER, 6) == 0` check alongside the existing `msg[0] == 0xF0` test (`src/KatanaLogic.h`).

- **C3**: ~~Preset cache is only updated during startup scan (`isScanning` guard); runtime effect changes from the amp are not cached.~~
  - **Fixed**: removed `isScanning` guard so `presetCache` is always updated when effect state is received (`src/KatanaLogic.h`).

**Performance:**

- **P1+Q1**: ~~`sendSysExChange()` used manual byte-by-byte address encoding and an intermediate `chkBuf[]` for checksum.~~
  - **Fixed**: added `static encodeAddr()` helper; rewritten to use `memcpy` for data and compute checksum directly from `&msg[8]` (`src/KatanaLogic.h`).

- **P2**: ~~`sendSysExRequest()` used manual byte-by-byte address/size encoding and an intermediate `tempForChk[]`.~~
  - **Fixed**: rewritten to use `encodeAddr()` for both address and size, checksum computed directly from `&msg[8]` (`src/KatanaLogic.h`).

- **P3**: ~~`printCenteredWordWrap()` called `displaySH.setTextSize(size)` inside the word-wrap loop (once per line).~~
  - **Fixed**: moved `setTextSize()` call before the loop (`src/oledControl.h`).

- **P4**: ~~`loop()` calls `millis()` up to 7 times per iteration.~~
  - **Fixed**: cached `millis()` once as `uint32_t nowMs` at top of `loop()`, used throughout (`src/main.cpp`).

**Code Quality:**

- **Q1**: See P1+Q1 above (combined with `encodeAddr` helper).

- **Q2**: ~~`toggleEffect()` had separate MUTE and non-MUTE branches each calling `sendSysExChange` with different data.~~
  - **Fixed**: computes `dataVal` in a single ternary expression, then makes one `sendSysExChange` call (`src/KatanaLogic.h`).

- **Q3**: ~~`checksum()` did not need access to `this`.~~
  - **Fixed**: made `static` (`src/KatanaLogic.h`).

- **Q4**: ~~`NameCallback` did not include the preset index, so the callback could not know which preset's name changed.~~
  - **Fixed**: changed typedef to `void (*)(int index, const char* name)`; updated `processCompleteSysEx` to pass `idx`; updated `onSysExNameChange` in `main.cpp` to refresh the main display when the active preset's name changes.

- **Q5**: ~~Panel name (index 0) was never fetched during startup scan.~~
  - **Fixed**: added `katana.requestNameByIndex(0)` + `waitForPatchLoad(150)` before the preset loop in `scanAllPresetsOnStartup()` (`src/main.cpp`).

- **Q6**: ~~`requestNameByIndex()` used a 9-case switch; `processCompleteSysEx()` used a 9-branch if-else chain for name address matching.~~
  - **Fixed**: added `const uint32_t nameAddr[9]` lookup array; `requestNameByIndex()` is now a bounds-checked array lookup; name matching in `processCompleteSysEx()` uses a loop over `nameAddr[]` (`src/KatanaLogic.h`).

**Build Quality:**

- **B1**: ~~`#endif` in `oledControl.h` was misplaced at line 105 (middle of `printCenteredWordWrap`) instead of end of file.~~
  - **Fixed**: moved `#endif` to actual end of file; fixed indentation (`src/oledControl.h`).

## Remaining Considerations

- Global objects and non-`inline` functions in header files still prevent safe multi-TU builds. Low risk while the project is a single `.cpp` file.
- Unnecessary redraws could be avoided when label or preset text has not changed.

## Build Verification

- All changes verified with `pio run -e esp32-s3-devkitc-1` -- zero errors, zero warnings.
- RAM: 6.2% (20,304 / 327,680 bytes) | Flash: 9.8% (326,761 / 3,342,336 bytes)

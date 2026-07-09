# OutEquip UART Protocol Verification

This file is a working lab sheet for the Summit2/OutEquip control board. Treat
the protocol table as a hypothesis until the live board proves it.

## Ground Rules

- Record raw UART values first, then map them to friendly HA behavior.
- Test one key at a time and wait for the raw sensor to settle.
- For every write, capture the requested key/value, the immediate response, the
  later queried raw value, and the physical behavior at the AC.
- When using the IR remote, do not press HA controls at the same time. Let the
  raw sensors reveal what the board changed internally.
- A successful write means the later queried raw value and the physical unit
  both match the requested behavior. A response frame alone is not enough.

## Raw Lab Controls

- `Raw UART Key`: protocol key number to probe.
- `Raw UART Value`: value to write for that key.
- `Raw UART Query`: sends the selected key with value `0`.
- `Raw UART Send`: sends the selected key with the selected raw value.

## Keys To Verify

| Key | Name | Test Values | What To Record |
| --- | ---- | ----------- | -------------- |
| 1 | Power | 1, 2 | Raw power, mode, LCD, compressor/fan state |
| 2 | Mode | 1, 2, 3, 4, 5, 6, 7 | Raw mode, raw fan, set temp, physical behavior |
| 3 | Set Temperature | 17-30, 61-86 | Raw set temp, front panel display, cooling behavior |
| 4 | Fan Speed | 1, 2, 3, 4, 5 | Raw fan, airflow, mode/preset side effects |
| 5 | Undervolt Protection | 90-119 | Raw undervolt, friendly cutoff, accepted clamp |
| 6 | Overvolt Protection | query only | Raw overvolt, friendly overvolt |
| 7 | Intake Air Temp | query only | Raw value, signed Celsius interpretation |
| 8 | Outlet Air Temp | query only | Raw value, signed Celsius interpretation |
| 10 | LCD | 0, 1, 2 | Raw LCD, physical display, power-state interaction |
| 16 | Swing | 1, 2 | Raw swing, confirm no Summit2 physical feature |
| 18 | Voltage | query only | Raw voltage, friendly voltage / 10 |
| 19 | Amperage | query only | Raw amperage, confirm always zero |
| 28 | Light | 1, 2 | Raw light, physical light, queried-state reliability |
| 66 | Active | 0, 1, 2 | Handshake behavior only |

## Preset/Fan Cross-Test

For each IR preset press and each HA/raw mode write, record raw mode, raw fan,
raw set temperature, and physical behavior.

| Action | Expected Raw Mode? | Values To Watch |
| ------ | ------------------ | --------------- |
| IR Cool/normal | 1 | mode, fan, set temp |
| IR Eco | 4 | mode, fan, set temp |
| IR Sleep | 5 | mode, fan, set temp |
| IR Turbo/Boost | 6 | mode, fan, set temp |
| HA preset None | unknown | mode, fan, set temp |
| HA preset Eco | unknown | mode, fan, set temp |
| HA preset Sleep | unknown | mode, fan, set temp |
| HA preset Boost | unknown | mode, fan, set temp |
| Raw fan 1-5 while Eco | unknown | mode, fan, set temp |
| Raw fan 1-5 while Boost | unknown | mode, fan, set temp |

## Current Confirmed Findings

- Undervolt protection accepts 90-119 decivolts, matching 9.0-11.9 V.
- Arbitrary set temperature writes appear rejected by this board firmware.
- IR preset changes do update the board's raw state.
- Raw fan speed `5` is Auto.
- Raw fan speed `4` is High when it latches.
- Raw fan speed `3` maps to Medium.
- Raw fan speeds `1` and `2` both currently map to Low in the HA bridge until
  physical airflow testing proves a better distinction.
- HA now exposes discrete custom fan modes `Fan 1`, `Fan 2`, `Fan 3`,
  `Fan 4`, and `Fan 5 Auto` so the board's raw choices can be commanded
  directly.
- Raw mode `5` latches Sleep over UART and changes raw fan to `2`.
- Raw mode `6` latches Boost/Turbo over UART and changes raw fan to `5`.
- Raw mode `4` did not latch Eco in the first raw UART test. It left raw mode
  at `1` and changed raw fan to `3`, so Eco should be treated as report-only
  until proven otherwise.
- Raw mode `4` also failed from a controlled Cool/fan-1 baseline. Sending raw
  set temperature `26` before raw mode `4` did not help; setpoint remained
  unchanged and mode stayed at `1`.
- IR Eco definitely reports raw mode `4`, raw set temperature `26`, and preset
  Eco. That means IR Eco and UART mode-value writes are not equivalent on this
  board firmware.
- IR target setpoints above 78 F appear to illuminate the Eco icon regardless
  of fan speed. Current captured example: 79 F / 26 C reports raw mode `4`,
  raw set temperature `26`, and Eco preset. This suggests Eco may be a
  controller-derived high-setpoint state rather than an independently writable
  UART mode.
- The Eco threshold is confirmed at the 78 F to 79 F boundary. IR setpoints at
  79 F and above illuminate the Eco icon regardless of fan speed.
- Pressing Sleep while Eco is active cancels the Eco icon and changes fan speed
  without changing the temperature setpoint. Pressing Sleep a second time
  resumes the previous fan speed and Eco mode.
- Pressing Turbo immediately drops the setpoint to 63 F and sets raw fan speed
  `5`. Pressing Turbo again clears the Turbo icon, leaves the setpoint at 63 F,
  and drops fan speed to Medium.
- Setting HA Cool should not force raw fan Auto; mode and fan are separate
  controls on this board.

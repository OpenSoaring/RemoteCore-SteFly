# SteFlyRemoteCore

Shared firmware core of the SteFly remote input devices for OpenSoar/XCSoar,
both built on an Arduino Leonardo (ATmega32U4):

* **Remote Stick** (`StickRemote-SteFly`): four buttons and a joystick.
* **Rotary Panel** (`RotaryPanel-SteFly`): three buttons and two rotary
  encoders with push buttons.

The devices differ only in how the navigation input is read. Everything else
has to behave identically, and lives in this library so that a fix or a new
feature reaches both devices at the same time:

* serial NMEA protocol (`$PSRC...`, see `src/RemoteCommunication.h`),
* the layout framework (row order, basic layout, dual-slot copy of the
  active layout in the EEPROM, pin configuration block;
  `src/RemoteLayouts.h`, `src/RemoteEEprom.h`),
* watchdog, early watchdog disable, reset log, clean reset,
* USB recovery (remote wakeup, debounced soft reattach, preemptive recovery),
* start sequence, serial number,
* Click / LongClick / DblClick handling of the push buttons and the extended
  key functions (mouse mode, STF/Vario toggle, app switch, app hide).

The code was moved here unchanged from `StickRemote-SteFly.ino` v3.0, so the
field-tuned timing of the Remote Stick applies to all devices.

## Layout rows

All devices use the same ten layout rows, so a key change via
`$PSRCK,S,Key,btn,...` addresses the same button on every device:

| Row | Name       | Remote Stick         | Rotary Panel                 |
|-----|------------|----------------------|------------------------------|
| 0   | `btnFn`    | Fn button            | Fn button                    |
| 1   | `btnMenu`  | M button             | M button                     |
| 2   | `btnX`     | X button             | X button                     |
| 3   | `btnSTF`   | STF button (back)    | push button of right encoder |
| 4   | `btnEnter` | joystick push        | push button of left encoder  |
| 5   | `joyUp`    | joystick up          | left encoder, one direction  |
| 6   | `joyDown`  | joystick down        | left encoder, other direction|
| 7   | `joyLeft`  | joystick left        | right encoder, one direction |
| 8   | `joyRight` | joystick right       | right encoder, other direction|
| 9   | `btnPTT`   | PTT (not read)       | not used                     |

For the rows 5..8, `Click` is the key sent in keyboard mode; `LongClick` and
`DblClick` hold the signed X and Y distance of one mouse step in mouse mode.

## Layouts

Only the basic layout (`src/layouts/basic-layout.h`) is part of the library.
It is identical on all devices and is the factory default at index 0. All
other layouts belong to the device sketch, which defines the table
`all_layouts[]` with `layout_basic` first. Devices that are used together can
so be tuned to each other, e.g. the Rotary Panel can leave a function to the
Remote Stick and use its buttons for something else. A layout number selected
with `$PSRCS,S,Layout,N` therefore refers to the layout table of the device
that receives the command.

## Installation

The library is header-only and has to be visible to the Arduino IDE as a
library, e.g. as a directory junction in the sketchbook:

```
Documents\Arduino\libraries\SteFlyRemoteCore -> C:\Projects\OpenSoaring\SteFly\SteFlyRemoteCore
```

Further libraries needed by the core: `Keyboard`, `Mouse`, `Bounce2`,
`r89m Buttons` and `r89m PushButton`; the Rotary Panel also needs `Encoder`
(PJRC).

## Usage

See the comment at the top of `src/SteFlyRemoteCore.h`. A device sketch
defines its identity strings, its factory pin mapping, its layout table and
the two hooks `remoteDeviceSetup()` and `remoteDeviceUpdate()`, and calls
`coreSetup()` and `coreLoop()` from `setup()` and `loop()`.

## Version

`STEFLY_REMOTE_CORE_VERSION` in `src/SteFlyRemoteCore.h` (and `version` in
`library.properties`) is reported by the device in the info block as
`$PSRCI,A,Core,<version>` and on request with `$PSRCI,R,Core`.

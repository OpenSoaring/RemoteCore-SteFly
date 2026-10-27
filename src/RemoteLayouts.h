#pragma once

// ============================================================================
//  Button indices and layout framework shared by all SteFly remote devices.
//
//  Every device uses the same ten layout rows in the same order, so a key
//  changed with $PSRCK,S,Key,btn,... addresses the same button on the Remote
//  Stick and on the Rotary Panel, the code handling the rows is shared, and
//  the EEPROM image of a layout has the same size and meaning on both. Which
//  key a row sends is up to the layouts, and apart from the basic layout
//  each device has its own.
//
//  Rows 0..4 are push buttons with Click, LongClick and DblClick actions.
//  Rows 5..8 are the four directions of the device's navigation input:
//    - Remote Stick: the four joystick contacts.
//    - Rotary Panel: the two directions of each rotary encoder (see the
//      enc* aliases in the Rotary Panel sketch).
//  For a direction row, Click is the key sent in keyboard mode, while the
//  LongClick and DblClick keys are reused as the signed X and Y distance of
//  one mouse step in mouse mode (see the JoyKey/JoyX/JoyY aliases).
//  Row 9 is reserved for the PTT button, which is wired to the radio and not
//  read by the firmware.
// ============================================================================

#include "RemoteKeys.h"

enum {
       btnFn, btnMenu, btnX, btnSTF,
       btnEnter,
       joyUp, joyDown, joyLeft, joyRight,
       btnPTT,
       btnSize
};

// The layout files are written row by row for exactly this enum, so a change
// of the row count must be reflected in every layout and in the EEPROM map.
static_assert(btnSize == 10, "The layouts and the EEPROM map expect 10 rows");

#define JoyKey Click
#define JoyX   LongClick
#define JoyY   DblClick

/* -----------------------------------------------------------------
 Keys used by the layouts.
 settings in XCSoar default.xci are
 F1 QuickMenu
 F2 Analysis
 F3 Checklist
 F4 FlarmTraffic
 F5 GotoLookup (Select Waypoint)
 F6 Setup Alternates
 F7 Setup Task
 F8 Setup Basic (wingload,bugs, QNH)
 F9
    Status all
    Calculator (tasks)

 settings in openvario.xci
 V:       for vario mode
 S:       for speed to fly mode
 M:       for vario menu (audio)
 X:       for quit or shutdown
 1,2,3,4: for btn_design (Pilot Event or something else)
------------------------------------------------------------------*/

// --- Layouts resident in flash ----------------------------------------------
// Only the basic layout is common to all devices; it is the factory default
// (index 0) that a device falls back to when its EEPROM holds no valid layout.
// All other layouts belong to the device sketch, because devices that are
// used together can be tuned to each other - e.g. the Rotary Panel leaves a
// function to the Remote Stick and uses its buttons for something else.
#include "layouts/basic-layout.h"

// Pointer table to all layouts of the device, defined by the sketch after
// including SteFlyRemoteCore.h (the table itself lives in flash as well):
//
//   const PROGMEM void* const all_layouts[] = { layout_basic, ... };
//   const uint8_t NUM_LAYOUTS = sizeof(all_layouts) / sizeof(all_layouts[0]);
//
// The index is what $PSRCS,S,Layout,N selects and what the EEPROM stores, so
// layout_basic must stay at index 0 and new layouts must only be appended,
// never inserted, or devices in the field would change meaning.
extern const PROGMEM void* const all_layouts[];
extern const uint8_t NUM_LAYOUTS;

// --- RAM working copy -------------------------------------------------------
// Both 'btn_design' and 'active_layout_index' are read/written by the main
// loop and by the EEPROM persistence layer in RemoteEEprom.h.
key_struct btn_design[btnSize];

uint8_t active_layout_index = 0;

// Layout loading is handled by RemoteEEprom.h:
//   loadLayoutFromEEPROM()        - boot-time restore from EEPROM
//   loadPresetFromFlash(idx)      - select a flash preset and persist
//   writeLayoutToEEPROM(idx)      - persist current btn_design
//   updateKeySlot(btn, slot, ...) - modify a single key
//   askKeySlot(btn, slot)         - read back a single key
//   sendLayoutHealth()            - NMEA $PSRCH,R diagnostic dump

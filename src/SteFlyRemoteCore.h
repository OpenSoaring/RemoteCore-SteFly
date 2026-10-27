#pragma once

// ============================================================================
//  SteFlyRemoteCore - shared firmware core of the SteFly remote devices
//  (Remote Stick, Rotary Panel) for OpenSoar/XCSoar on an Arduino Leonardo.
//
//  The library is header-only on purpose: the core needs to see the
//  device sketch's configuration (REMOTE_PUSHBUTTON_COUNT) and calls hooks
//  that the sketch defines, so it has to be compiled in the sketch's own
//  translation unit. Include this header exactly once, from the .ino file.
//
//  Usage in a device sketch:
//
//    #define REMOTE_PUSHBUTTON_COUNT (btnEnter + 1)   // optional, default btnSize
//    #include <SteFlyRemoteCore.h>
//
//    const char* Device  = "...";
//    const char* Version = "...";
//    const char* Date    = __DATE__;
//    const char* FileName = ...;              // from __FILE__ of the sketch
//    const char DefaultSerialNumber[] = "SF-...:00001";
//    const PROGMEM uint8_t default_pin_config[btnSize] = { ... };
//    const PROGMEM void* const all_layouts[] = { layout_basic, ... };
//    const uint8_t NUM_LAYOUTS = sizeof(all_layouts) / sizeof(all_layouts[0]);
//
//    void remoteDeviceSetup()  { ... }        // device specific inputs
//    void remoteDeviceUpdate() { ... }
//    void setup() { coreSetup(); }
//    void loop()  { coreLoop(); }
// ============================================================================

#define STEFLY_REMOTE_CORE_VERSION "1.0.0"

#include "RemoteCore.h"

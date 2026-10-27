#pragma once

// ============================================================================
//  Device independent runtime of the SteFly remote devices.
//
//  Everything that must behave identically on the Remote Stick and on the
//  Rotary Panel lives here: watchdog, reset log, USB recovery, the start
//  sequence, the Click/LongClick/DblClick handling of the push buttons, the
//  extended key functions and the serial command loop. The device sketch only
//  adds what depends on its hardware (joystick or rotary encoders) through
//  the two hooks remoteDeviceSetup() and remoteDeviceUpdate().
//
//  The code was moved here unchanged from StickRemote-SteFly.ino v3.0; the
//  timing values below are the field-tuned values of that version.
// ============================================================================

#include <Arduino.h>
#include <Mouse.h>
#include <Keyboard.h>
#include <PushButton.h>
#include <EEPROM.h>
#include <avr/wdt.h>             // wdt_enable, wdt_disable, wdt_reset, WDTO_*

#include "RemoteLayouts.h"
#include "RemoteEEprom.h"        // dual-slot EEPROM persistence (uses btn_design + all_layouts)
#include "RemoteCommunication.h"

// Placement-new operator. Needed because the r89m/PushButton library does
// not expose Bounce::attach(), so buttons are bound to their pins by
// constructing the PushButton objects in place at runtime (after the
// EEPROM-stored pin assignment is loaded in setup()). Device sketches use it
// for their own input objects as well.
inline void* operator new(size_t, void* ptr) noexcept { return ptr; }

// All feature flags are always active since v3.0:
#define RESTART_VERSION          // staggered boot path with serial 'Start' frame
#define WITHOUT_MOUSE            // no Mouse.* methods even if Mouse.h is included
#define WAIT_FOR_ENTER           // setup() waits for Enter to be released before continuing
#define WITH_DBLCLICK            // per-button DblClick handling
// USB recovery package - works with modified Caterina bootloader or stock one
// (the stock Caterina just adds an 8s wait on WDT reset, which WITH_USB_REATTACH
// avoids by re-enumerating via USBDevice.detach()/attach() instead).
#define WITH_EARLY_WDT_DISABLE   // .init3 hook: MCUSR=0; wdt_disable() before setup()
#define WITH_RESET_LOG           // MCUSR + reset counter logged to EEPROM, NMEA boot msg
#define WITH_USB_REATTACH        // soft re-enumerate on USB suspend / send stall
#define WITH_CLEAN_RESET         // exit(1) -> wdt_enable(WDTO_15MS); while(1){}

// Number of layout rows that are read as push buttons. The Remote Stick reads
// all rows as push buttons (joystick contacts included); the Rotary Panel
// reads only rows 0..btnEnter and handles the direction rows with encoders.
#ifndef REMOTE_PUSHBUTTON_COUNT
# define REMOTE_PUSHBUTTON_COUNT btnSize
#endif

// --- Provided by the device sketch -------------------------------------------
// Identity strings (also declared in RemoteCommunication.h).
extern const char *Version;
extern const char *Device;
extern const char *Date;
extern const char *FileName;
// Serial number written to a blank EEPROM, e.g. "SF-SRC:00002".
extern const char DefaultSerialNumber[];
// Called once in setup() after the push buttons are bound, to set up the
// device specific inputs (joystick callbacks, rotary encoders).
void remoteDeviceSetup();
// Called in every loop() pass once the start delay has elapsed, right after
// the push buttons have been updated.
void remoteDeviceUpdate();

// define timing for buttons etc.
const int button_hold_threshold = 900;
const int enter_hold_threshold  = 2000;
const int dblclick_threshold    = 350;  // ms - wait window after a first tap to detect a second one.
                                        // A shorter value feels snappier, a longer one is more forgiving
                                        // for pilots wearing gloves. 350ms is a field-tuned middle ground.
const int strongclick_threshold = 250;  // ms - if the button was held longer than this before release,
                                        // it can't have been a rapid double-tap intent -> fire SingleClick
                                        // immediately without waiting for a possible second click.
const unsigned long start_delay = 500;  // ms
const unsigned int start_wait   = 2000;  // 2000;

// define variables
bool mouse_active = false;

#ifdef WITH_DBLCLICK
unsigned long BtnClickTime[btnSize] = {0,0,0,0,0,0,0,0,0,0};
unsigned long BtnPressTime[btnSize] = {0,0,0,0,0,0,0,0,0,0};
unsigned long DblClickTime[btnSize] = {0,0,0,0,0,0,0,0,0,0};
#endif

bool startBit = false;
unsigned long startTime = 0;
unsigned long time_now = 0;

unsigned long timeWDT = 0;

// Serial number lives here because setup() initializes it from EEPROM
// on first boot. The send buffers and read_buffer are defined inside
// RemoteCommunication.h.
char serialNum[20];

sKey STF_key = {0,'V'};

const uint8_t pullup = ENABLE_INTERNAL_PULLUP;

// The push buttons are constructed in setup() (see coreSetup()), because
// their pins are only known after the pin configuration has been read from
// the EEPROM. Raw storage instead of an array of objects avoids constructing
// them twice and works for any REMOTE_PUSHBUTTON_COUNT.
alignas(PushButton) static uint8_t button_storage[REMOTE_PUSHBUTTON_COUNT * sizeof(PushButton)];
PushButton* const button = reinterpret_cast<PushButton*>(button_storage);

void sendKey(const sKey key);
void SingleClick(int index);
bool DblClickTest(int index);
void onButtonClick(PushButton& btn);
void onLongClick(PushButton& btn);
void onReset(PushButton& btn);
void resetCmd();
void switchMouse();
void toggleSpeedToFly_Vario();
void switchApp();
void switchApp_doubleTAB();
void hideApp();

#ifdef WITH_EARLY_WDT_DISABLE
// ----------------------------------------------------------------------------
// Runs BEFORE setup() and BEFORE the C++ global constructors (section .init3).
// It clears MCUSR and disables the watchdog at once, so that a previous WDT
// reset is guaranteed to get through the Caterina bootloader. Even if an
// unmodified Caterina still waits 8s afterwards, it cannot trip over the
// watchdog again.
// mcusr_at_boot keeps its value across the reset (.noinit).
uint8_t mcusr_at_boot __attribute__((section(".noinit")));

void wdtEarlyDisable(void) __attribute__((naked, used, section(".init3")));
void wdtEarlyDisable(void) {
    mcusr_at_boot = MCUSR;
    MCUSR = 0;
    wdt_disable();
}
#endif  // WITH_EARLY_WDT_DISABLE

// ----------------------------------------------------------------------------
void watchdogInit() {
        timeWDT = time_now = millis();
        wdt_reset();
        WDTCSR |= (1 << WDCE) | (1 << WDE);  // set relevant bits to enter watchdog timer configuration mode
        WDTCSR = (1<< WDE) | WDTO_2S;        // turn on WatchDog timer
}
// ----------------------------------------------------------------------------
void wdtReset() {
    time_now = millis();  // must be refreshed here, callers rely on it
    if (time_now - timeWDT >= 450) {  // 2 in 1 second
        timeWDT = time_now;
        wdt_reset();
    }
}

#ifdef WITH_USB_REATTACH
void softReattach(const char *reason);

// ----------------------------------------------------------------------------
// Number of soft reattaches since boot (NMEA diagnostics).
static uint8_t       reattachCount       = 0;
// Time of the last soft reattach (anti-flapping).
static unsigned long lastReattachMillis  = 0;
// Minimum pause between two reattaches.
static const unsigned long REATTACH_MIN_INTERVAL = 5000;

// --- Stabilisation (v3.0) -------------------------------------------------
// Time at which the USB state was first seen as 'abnormal'
// (isSuspended() or !configured()). 0 = currently normal.
static unsigned long usbAbnormalSince    = 0;
// Minimum time the abnormal state has to last before a reattach is
// triggered. Protects against spurious events caused by electrical glitches
// (USBasp on the ICSP header, ESD, EMI in flight).
static const unsigned long USB_ABNORMAL_DEBOUNCE   = 500;
// If the device stays abnormal for longer than this *without* a key being
// pressed, a reattach is started proactively from the main loop.
static const unsigned long USB_PREEMPTIVE_RECOVERY = 5000;

// Is the current USB state suspicious?
static inline bool usbAbnormalNow() {
    return USBDevice.isSuspended() || !USBDevice.configured();
}

// Has the abnormal state lasted longer than the debounce threshold?
static inline bool usbAbnormalStable() {
    return (usbAbnormalSince != 0) &&
           (time_now - usbAbnormalSince >= USB_ABNORMAL_DEBOUNCE);
}

// Called once per main loop pass. Tracks the moment the interface first
// became abnormal and triggers a preemptive reattach if it stays that way
// for too long.
static void monitorUsbState() {
    if (usbAbnormalNow()) {
        if (usbAbnormalSince == 0) {
            usbAbnormalSince = time_now;
        } else if (time_now - usbAbnormalSince >= USB_PREEMPTIVE_RECOVERY) {
            softReattach("idle-recover");
            // softReattach has its own anti-flapping (REATTACH_MIN_INTERVAL);
            // reset the marker so that the next episode is debounced again.
            usbAbnormalSince = 0;
        }
    } else {
        usbAbnormalSince = 0;
    }
}

// Tries a soft USB re-enumeration without resetting the chip.
// Advantage over a WDT reset: no bootloader phase, recovery < 1s, and the
// running state is kept (button state, EEPROM counters etc.).
void softReattach(const char *reason) {
    unsigned long now = millis();
    if (lastReattachMillis != 0 && (now - lastReattachMillis) < REATTACH_MIN_INTERVAL) {
        return;
    }
    lastReattachMillis = now;
    if (reattachCount < 255) reattachCount++;

    char buf[48];
    snprintf(buf, sizeof(buf), "Reattach,%s,n=%u", reason, (unsigned)reattachCount);
    sendNMEA('I', buf);
    Serial.flush();

    USBDevice.detach();
    // Wait without losing the main loop: keep kicking the WDT.
    for (uint8_t i = 0; i < 25; i++) {  // ~250 ms detach
        wdt_reset();
        delay(10);
    }
    USBDevice.attach();
    // Wait up to ~300 ms until the host has enumerated again.
    for (uint8_t i = 0; i < 30; i++) {
        wdt_reset();
        delay(10);
        if (USBDevice.configured()) break;
    }
}
#endif  // WITH_USB_REATTACH

// NMEA helpers (NMEAChecksum, VerifyNMEAChecksum, AppendNMEAChecksum,
// sendNMEA) live in RemoteCommunication.h.

// ----------------------------------------------------------------------------
void sendKey(const sKey key) {

#ifdef WITH_USB_REATTACH
    // If the host has suspended the interface: request a remote wakeup
    // first, then wait briefly. If that does not help AND the state has
    // already been abnormal for a while (debounce against spurious events,
    // e.g. a connected USBasp or short EMI) -> soft reattach.
    if (USBDevice.isSuspended()) {
        bool woken = USBDevice.wakeupHost();
        for (uint8_t i = 0; i < 20 && USBDevice.isSuspended(); i++) {
            wdt_reset();
            delay(5);
        }
        if (USBDevice.isSuspended() && usbAbnormalStable()) {
            softReattach(woken ? "wake-fail" : "no-wake");
        }
    } else if (!USBDevice.configured() && usbAbnormalStable()) {
        // Interface not configured AND stable in that state for a while -> reattach.
        softReattach("not-cfg");
    }
#endif  // WITH_USB_REATTACH
    if (key.ext) {
        switch (key.key) {
          case MOUSE_SWITCH:    switchMouse(); break;
          case APP_SWITCH:      switchApp(); break;
          case APP_SWITCH2:     switchApp_doubleTAB(); break;
          case APP_HIDE:        hideApp(); break;
          case TOOGLE_STF_VARIO:toggleSpeedToFly_Vario(); break;
          default: break;
        }
      // keyEvent(key);
      // sprintf(send_buffer+strlen(send_buffer), "%02X.%02X", key.ext, key.key);
    } else {
      Keyboard.write(key.key);
      char *keycode = key_code(key);
      if (keycode) strcpy(send_buffer, keycode);
      else         send_buffer[0] = 0;
    }
    if (*send_buffer) {
      sendNMEA('K', send_buffer);
      send_buffer[0] = 0;
    }
}

// ----------------------------------------------------------------------------
// Extended Key Functions:
// ----------------------------------------------------------------------------
void switchMouse() {
    mouse_active = !mouse_active;
    sprintf_P(send_buffer+strlen(send_buffer), mouse_active ? PSTR("Mouse") :PSTR("Arrow"));
}

// ----------------------------------------------------------------------------
void toggleSpeedToFly_Vario() {
  // use a SWITCH to enable Vario or Speed To Fly mode
  STF_key.key = (STF_key.key == 'V') ? 'S' : 'V';
  sendKey(STF_key);
}

// ----------------------------------------------------------------------------
// Switch to the previous app (Alt+Tab style).
//
// Cross-platform timing notes:
//   * Windows / Linux: switch happens immediately on ALT+TAB press,
//     ALT-release just confirms. Total time doesn't matter much.
//   * Android 13: switcher is shown while ALT is held, app is committed
//     on ALT-release. Total of ~300ms feels responsive.
//   * Android 11: requires ALT to be held BEFORE TAB is pressed for the
//     gesture to register as "app switch" (not "alt typed, then tab typed").
//     Pre-ALT hold of >=50ms greatly improves reliability there.
//
// IMPORTANT: this implementation sends a SINGLE TAB while ALT is held.
// On desktop OSes that moves one step in the MRU stack (= previous app).
// A double-TAB inside the same ALT-hold would move TWO steps on Windows/Linux
// (= switch to a different app than intended). Only on certain Android versions
// might rapid double-TAB stabilize the switch - not portable.
// If you want to experiment with double-TAB on Android-only devices, swap to
// switchApp_doubleTAB() defined below and observe behavior.
void switchApp() {
       Keyboard.press(KEY_LEFT_ALT);
       delay(60);                       // pre-hold: let Android 11 register the modifier
       Keyboard.press(KEY_TAB);
       delay(50);                       // TAB press: long enough to register on all platforms
       Keyboard.release(KEY_TAB);
       // Post-hold: keep ALT down so the switcher stays open while the
       // OS commits to the target app. WDT-kick during the wait.
       for (uint8_t i = 0; i < 50; i++) {
       // for (uint8_t i = 0; i < 28; i++) {
         wdt_reset();
         delay(10);
       }
       Keyboard.release(KEY_LEFT_ALT);
}

// ----------------------------------------------------------------------------
// EXPERIMENTAL variant: double-TAB inside the same ALT-hold.
// On Windows/Linux this switches to the THIRD most recent app (skips one).
// On some Android versions this gives a more reliable switch to the previous
// app than single-TAB. Untested across the full Android version matrix.
// Reachable through the extended key APP_SWITCH2 (KEY_APP_SWITCH2).
void switchApp_doubleTAB() {
       Keyboard.press(KEY_LEFT_ALT);
       delay(60);
       Keyboard.press(KEY_TAB);
       delay(50);
       Keyboard.release(KEY_TAB);
       delay(100);                       // inter-TAB gap
       Keyboard.press(KEY_TAB);
       delay(50);
       Keyboard.release(KEY_TAB);
       for (uint8_t i = 0; i < 25; i++) {
         wdt_reset();
         delay(10);
       }
       Keyboard.release(KEY_LEFT_ALT);
}

// ----------------------------------------------------------------------------
// Hide / minimize the current app (WIN+ENTER style).
//
// Same timing strategy as switchApp(): pre-hold of the GUI modifier for
// 60ms so Android 11 reliably registers the modifier before ENTER comes,
// 50ms ENTER press for guaranteed key-down delivery, then 200ms post-hold
// to let the OS commit the action before the GUI key is released.
// Total ~310ms - faster than the previous ~225ms (which itself was already
// short, but the missing pre-hold caused intermittent failures on Android 11).
void hideApp() {
       Keyboard.press(KEY_LEFT_GUI);
       delay(60);                       // pre-hold: let Android 11 register the modifier
       Keyboard.press(KEY_RETURN);
       delay(50);                       // ENTER press: long enough on all platforms
       Keyboard.release(KEY_RETURN);
       for (uint8_t i = 0; i < 20; i++) {
         wdt_reset();
         delay(10);
       }
       Keyboard.release(KEY_LEFT_GUI);
}

// ----------------------------------------------------------------------------
// To be called from the sketch's setup().
void coreSetup() {
  Serial.begin(9600);

  // Load pin assignment (hardware mapping) from EEPROM. On first boot or
  // corruption, falls back to factory default and persists it. Must happen
  // before the buttons are constructed below and before WAIT_FOR_ENTER
  // reads pin state.
  loadPinConfig();

  // Bind each PushButton to its actual MCU pin. Because PushButton has no
  // attach() method exposed, the objects are constructed in place with the
  // correct pin. Unconnected buttons (pin 0xFF) get the harmless dummy pin 13
  // (= LED_BUILTIN) - the main loop ignores them anyway via the layout's
  // Click/LongClick/DblClick == 0 check.
  for (uint8_t i = 0; i < REMOTE_PUSHBUTTON_COUNT; i++) {
      new (&button[i]) PushButton(active_pin_config[i] != 0xFF ? active_pin_config[i] : 13, pullup);
  }

  // Restore layout from EEPROM with dual-slot health-voting + self-heal.
  // If both slots are corrupt or never initialized, fall back to flash preset 0.
  if (!loadLayoutFromEEPROM()) {
    loadPresetFromFlash(0);
  }

#ifdef WAIT_FOR_ENTER
  // wait until button 'Enter' is released
  pinMode(active_pin_config[btnEnter], INPUT_PULLUP);
  while (!digitalRead(active_pin_config[btnEnter])) { // = isPressed
    wdtReset();
    delay(200);
  }
  button[btnEnter].update();
#endif

/* TODO(August2111): This is a (bad) workaround for strcpy: the first cmd
   in source code - not in calling behaviour - have a bit failure inside...:*/
  // strncpy_P(send_info, PSTR("zzzzzzzzzzzzzzzzzzzzzzzz"), buffer_size);
  memset(send_info, 'z', buffer_size);
  send_info[buffer_size-1] = 0;

  // normal key buttons:
  for (int i = 0;i < btnEnter; i++) {
    button[i].onRelease(0,button_hold_threshold-1, onButtonClick);
    button[i].onHold(button_hold_threshold, onLongClick);
  }
  // Enter has a different threshold:
  button[btnEnter].onRelease(0,enter_hold_threshold-1, onButtonClick);
  button[btnEnter].onHold(enter_hold_threshold, onLongClick);

  // joystick, rotary encoders, ...
  remoteDeviceSetup();

  Keyboard.begin();
  Mouse.begin();

  startTime = millis();  // overflow after ~50 days
}

// ----------------------------------------------------------------------------
// All send/dispatch functions (sendVersion, sendDevice, sendFileDate,
// sendFileName, sendTime, sendLayout, sendSerialNumber, sendSettingsBlock,
// sendInfoBlock, queryInfo, execCommand) live in RemoteCommunication.h.
// Only resetCmd() stays here since it belongs to the reset paths
// (clean WDT-driven reboot).
// ----------------------------------------------------------------------------
void resetCmd() {
    sendNMEA('I', "Reset(Serial)");
    Serial.flush();
    wdt_enable(WDTO_15MS);    // clean reset via watchdog
    while (1) { }
}

// ----------------------------------------------------------------------------
void SingleClick(int index) {
  if (btn_design[index].Click.key) { // [KEY_PRESSED]) {
    if (mouse_active && index == btnEnter) {
      Mouse.release(MOUSE_LEFT);
      // Mouse.press(MOUSE_LEFT);
    } else
      sendKey(btn_design[index].Click);
  }
}
// ----------------------------------------------------------------------------
// To be called from the sketch's loop().
void coreLoop() {
  time_now = millis();

#if defined(WITH_DBLCLICK)
  for (int index = 0; index <= btnEnter; index++) {
    if  (!digitalRead(active_pin_config[index])  // = isPressed
    // if (button[index].isPressed()
        && (BtnPressTime[index] == 0))
        BtnPressTime[index] = time_now;
    int click_time = time_now - BtnClickTime[index];
    if (BtnClickTime[index] && (click_time > dblclick_threshold)) { //-> SingleClick!
       // Serial.println(time_now - BtnClickTime[index]);  /* TEST */
       BtnClickTime[index] = 0;
       SingleClick(index);
    }
  }
#endif // defined(WITH_DBLCLICK)

#if !defined(RESTART_VERSION)
  for (int i=0;i<REMOTE_PUSHBUTTON_COUNT;i++)
    button[i].update();
  remoteDeviceUpdate();
#else // RESTART_VERSION

  if (!startBit) {
      if (time_now-startTime > start_wait) {
         startBit = true;
         sendNMEA('I',"Start");
         serialNum[0] = EEPROM.read(0);
         if (serialNum[0] == 'S') {
             // maybe a valid serial SteFly number
            EEPROM.get(0, serialNum);
         } else {
            strncpy(serialNum, DefaultSerialNumber, sizeof(serialNum) - 1);
            serialNum[sizeof(serialNum) - 1] = 0;
            EEPROM.put(0, serialNum);
            Serial.println("Rite EEProm");
         }
         sendNMEA('I',serialNum);

# ifdef WITH_RESET_LOG
         // Reset statistics in the EEPROM (offset 64, outside the serial number).
         // Layout: [64]='R' magic, [65..66]=uint16 resetCount, [67]=last MCUSR.
         {
           const int RL_BASE = 64;
           const uint8_t RL_MAGIC = 'R';
           uint16_t resetCount = 0;
           uint8_t  lastMcusr  = mcusr_at_boot;
           if (EEPROM.read(RL_BASE) != RL_MAGIC) {
             EEPROM.write(RL_BASE, RL_MAGIC);
             resetCount = 1;
             EEPROM.put(RL_BASE + 1, resetCount);
           } else {
             EEPROM.get(RL_BASE + 1, resetCount);
             if (resetCount < 0xFFFF) resetCount++;
             EEPROM.put(RL_BASE + 1, resetCount);
           }
           EEPROM.write(RL_BASE + 3, lastMcusr);

           const char *cause;
           if      (lastMcusr & (1<<WDRF))  cause = "WDT";
           else if (lastMcusr & (1<<BORF))  cause = "BOR";
           else if (lastMcusr & (1<<EXTRF)) cause = "EXT";
           else if (lastMcusr & (1<<PORF))  cause = "POR";
           else                             cause = "UNK";
           char buf[60];
           snprintf(buf, sizeof(buf),
                    "Reset=%s,MCUSR=0x%02X,n=%u",
                    cause, lastMcusr, (unsigned)resetCount);
           sendNMEA('I', buf);
         }
# endif  // WITH_RESET_LOG
        // Enabling after start_wait time
        watchdogInit();
      }
  } else {
    for (int i=0;i<REMOTE_PUSHBUTTON_COUNT;i++)
      button[i].update();
    remoteDeviceUpdate();
  }
#endif

  wdtReset();

#ifdef WITH_USB_REATTACH
  // Watch the USB state: stability debouncer and preemptive recovery
  monitorUsbState();
#endif

  // Stream-based NMEA reader + dispatcher lives in RemoteCommunication.h.
  // processSerialChar() accumulates chars into read_buffer and dispatches a
  // complete '$PSRC...' frame on newline via dispatchSerialCommand().
  //
  // The legacy single-char shortcuts (v, d, u, f, t, 0..2) have been retired -
  // same functionality is reachable via:
  //   v    -> $PSRCI,R,Version
  //   d    -> $PSRCI,R,Device
  //   u    -> $PSRCI,R,FileDate
  //   f    -> $PSRCI,R,FileName
  //   t    -> $PSRCI,R,Time
  //   0..2 -> $PSRCS,S,Layout,0..2
  int c = Serial.read();
  if (isprint(c) || c == '\n') {
    processSerialChar(c);
  }
}

// ----------------------------------------------------------------------------
#ifdef WITH_DBLCLICK
// Called on every button release. Decides:
//   a) If this is the second click within dblclick_threshold and the button
//      has a DblClick action configured -> fire DblClick now, suppress the
//      pending Click that was deferred from the first release.
//   b) If the button has NO DblClick action -> fire Click IMMEDIATELY,
//      no latency penalty. Most buttons fall into this category.
//   c) Otherwise: first click on a DblClick-capable button -> defer the
//      Click until the timeout in coreLoop() decides "no second click coming".
bool DblClickTest(int index) {
  if (BtnPressTime[index] != 0) {
    int press_time = time_now - BtnPressTime[index];
    BtnPressTime[index] = 0;  // reset time
    if (press_time > strongclick_threshold) {  // threshold for strong click
           SingleClick(index);
         /*
         char buf[48];
         snprintf_P(buf, sizeof(buf), PSTR("StrongClick: t=%u"), press_time);
         sendNMEA('I', buf);
         */
           BtnClickTime[index] = 0;
           return true;
    }
  }
  if (BtnClickTime[index] > 0) {
     // --- (a) Second click within window -> DblClick ---
     int click_time = time_now - BtnClickTime[index];
     DblClickTime[index] = time_now;
     BtnClickTime[index] = 0;             // cancel the pending single-Click
     if (btn_design[index].DblClick.key)
       sendKey(btn_design[index].DblClick);
     return true;
  }

  // --- (b) No DblClick configured -> eager Click, no waiting ---
  if (btn_design[index].DblClick.key == 0) {
    if (btn_design[index].Click.key) {
      if (mouse_active && index == btnEnter) {
        Mouse.click(MOUSE_LEFT);
      } else {
        sendKey(btn_design[index].Click);
      }
    }
    DblClickTime[index] = time_now;       // bookkeeping, not used for trigger
    return false;
  }

  // --- (c) DblClick is configured -> defer the Click for now ---
  if (time_now - DblClickTime[index] > 1000) {
      BtnClickTime[index] = time_now;
      if (mouse_active && index == btnEnter) {
         Mouse.press(MOUSE_LEFT);
      }
  }
  return false;
}
#endif

// ----------------------------------------------------------------------------
void onButtonClick(PushButton& btn){

#ifdef WITH_DBLCLICK
  DblClickTest(&btn - button);
#endif  // WITH_DBLCLICK
}

// ----------------------------------------------------------------------------
void onReset(PushButton& btn) {
    sendNMEA('I', "Reset(Manually)");

  while (!digitalRead(active_pin_config[btnEnter])) { // = isPressed
    wdtReset();
    delay(200);
  }

#ifdef WITH_CLEAN_RESET
    // Clean reset: arm the WDT briefly, then busy-loop. No cli() as in
    // exit(), so the USB detach sequence in the bootloader runs normally.
    Serial.flush();
    wdt_enable(WDTO_15MS); // much faster...
    while (1) { }
#else
    exit(1);
#endif
}

// ----------------------------------------------------------------------------
void onBtnHeldTopLeft(PushButton& btn){
  int index = &btn - button;
  sendKey(btn_design[index].LongClick);
}

// ----------------------------------------------------------------------------
void onLongClick(PushButton& btn){
  int index = &btn - button;
  BtnPressTime[index] = 0;
  if (index == btnEnter) {
    onReset(btn);
  } else {
    if (btn_design[index].LongClick.key) {
#ifdef WITH_DBLCLICK
      BtnClickTime[index] = 0;
#endif  // WITH_DBLCLICK
      sendKey(btn_design[index].LongClick);
    }
  }
}

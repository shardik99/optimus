#include <Keyboard.h>
#include <avr/wdt.h>

#define logEvent(a, b) dologEvent(a,  __FUNCTION__, b)

const int relayPin = 7;
const int relayLedPin = 6;
const int kbLedPin = 5;
const int overrideLed = 13;
const int pwrOverride = 4;

int debugState = 0;
int lockoutState = 0;
int wdtEnable = 0;

unsigned long lastResetLogTime = 0;

String command = "";

// Logs tagged messages with time (weeks:days hh:mm:ss format)
void dologEvent(const String &tag, const String &func, const String &msg) {
  // If debug log, only show if debugState is enabled
  if (tag == "DBG" && !debugState)
    return;

  unsigned long currentMillis = millis();       //unsigned long

  // Throttle debug logs: allow only if 5 seconds passed since last
  if (tag == "DBG" && (currentMillis - lastResetLogTime < 5000))
    return;

  if (tag == "DBG")
    lastResetLogTime = currentMillis;

  unsigned long seconds = currentMillis / 1000; //unsigned long
  unsigned long minutes = seconds / 60;         //unsigned long
  unsigned long hours = minutes / 60;           //unsigned long
  unsigned long days = hours / 24;              //unsigned long
  unsigned long weeks = days / 7;
  currentMillis %= 1000;
  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  days %= 7;

  // Print log
  Serial1.print("[");
  Serial1.print(tag);
  Serial1.print("] W");
  Serial1.print(weeks);
  Serial1.print("d");
  Serial1.print(days);
  Serial1.print(": ");
  Serial1.print(hours);
  Serial1.print(":");
  Serial1.print(minutes);
  Serial1.print(":");
  Serial1.print(seconds);
  Serial1.print(" ");
  if (debugState) {
    Serial1.print(func);
    Serial1.print(" ");
  }
  Serial1.println(msg);
}

void safeDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    delay(50);
    if (wdtEnable){
      wdt_reset();
    }
  }
}

// Simulate combo keypresses
void pressCombo(uint8_t key1, uint8_t key2, const String &desc) {
  logEvent("KB", "Pressing " + desc);
  digitalWrite(kbLedPin, HIGH);
  Keyboard.press(key1);
  safeDelay(50);
  Keyboard.press(key2);
  safeDelay(100);
  Keyboard.releaseAll();
  digitalWrite(kbLedPin, LOW);
}

// Simulate single key press and release
void keyPress(uint8_t key, const String &keyName) {
  digitalWrite(kbLedPin, HIGH);
  logEvent("KB", "Pressing key: " + keyName);
  Keyboard.press(key);
  safeDelay(100);
  Keyboard.release(key);
  logEvent("KB", "Released key: " + keyName);
  digitalWrite(kbLedPin, LOW);
}

// Simulate character key stroke (printable characters only)
void keyStroke(char keyName) {
  digitalWrite(kbLedPin, HIGH);
  logEvent("KB", "Pressing key: " + keyName);
  Keyboard.write(keyName);
  safeDelay(100);
  digitalWrite(kbLedPin, LOW);
}

// Activates a relay for the given duration (ms)
void triggerRelay(unsigned int durationMs) {
  logEvent("SYS", "Activating relay for " + String(durationMs) + " ms");
  digitalWrite(relayLedPin, HIGH);
  digitalWrite(relayPin, LOW);
  safeDelay(durationMs);
  digitalWrite(relayPin, HIGH);
  digitalWrite(relayLedPin, LOW);
  logEvent("SYS", "Relay deactivated");
}

// Flashes a given LED N times
void flashLed(int led, int count) {
  for (int ctr = 0; ctr < count; ctr++) {
    digitalWrite(led, HIGH);
    safeDelay(200);
    digitalWrite(led, LOW);
    safeDelay(200);
  }
}

// Power command handling: PRESS, HOLD, REBOOT
void processPowerCommand(String cmd) {
  if (cmd == "PRESS") {
    logEvent("PWR", "Pressing power button");
    triggerRelay(200);
  } else if (cmd == "HOLD") {
    logEvent("PWR", "Holding power button");
    triggerRelay(5000);
  } else if (cmd == "REBOOT") {
    logEvent("PWR", "Holding power button");
    triggerRelay(5000);
    logEvent("PWR", "Pressing power button");
    triggerRelay(200);
  } else {
    logEvent("PWR", "Unknown power command: " + cmd);
  }
}

// Sends WIN+P, then selects display mode via DOWN arrow N times
void switchDisplay(int downCount, const String &label) {
  logEvent("KB", label);
  digitalWrite(kbLedPin, HIGH);

  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('p');  // Open display switch menu
  safeDelay(500);
  Keyboard.releaseAll();
  safeDelay(500);

  for (int i = 0; i < downCount; i++) {
    Keyboard.write(KEY_DOWN_ARROW);
    safeDelay(100);
  }

  Keyboard.write(KEY_RETURN);  // Confirm selection
  Keyboard.write(KEY_ESC);     // Dismiss overlay
  digitalWrite(kbLedPin, LOW);
}

// Keyboard action handling
void processKeyboardCommand(String cmd) {
  if (cmd == "FUNC12") {
    keyPress(KEY_F12, "F12");
  } else if (cmd == "FUNC11") {
    keyPress(KEY_F11, "F11");
  } else if (cmd == "DEL") {
    keyPress(KEY_DELETE, "Delete");
  } else if (cmd == "BKSP") {
    keyPress(KEY_BACKSPACE, "Backspace");
  } else if (cmd == "ESC") {
    keyPress(KEY_ESC, "Escape");
  } else if (cmd == "RET") {
    keyPress(KEY_RETURN, "Enter");
  } else if (cmd == "UP") {
    keyPress(KEY_UP_ARROW, "Up Arrow");
  } else if (cmd == "DOWN") {
    keyPress(KEY_DOWN_ARROW, "Down Arrow");
  } else if (cmd == "LEFT") {
    keyPress(KEY_LEFT_ARROW, "Left Arrow");
  } else if (cmd == "RIGHT") {
    keyPress(KEY_RIGHT_ARROW, "Right Arrow");
  } else if (cmd == "WS") {
    keyStroke(' ');
  } else if (cmd == "WIN") {
    keyPress(KEY_LEFT_GUI, "Windows");
  } else if (cmd == "CTRL") {
    keyPress(KEY_LEFT_CTRL, "Control");
  } else if (cmd == "ALT") {
    keyPress(KEY_LEFT_ALT, "Alt");
  } else if (cmd == "SHIFT") {
    keyPress(KEY_LEFT_SHIFT, "Shift");
  } else if (cmd == "TAB") {
    keyPress(KEY_TAB, "Tab");
  } else if (cmd == "QUIT") {
    logEvent("KB", "Keyboard window quit");
    pressCombo(KEY_LEFT_ALT, KEY_F4, "Alt+F4");
  } else if (cmd == "SWITCH") {
    logEvent("KB", "Keyboard window switch");
    pressCombo(KEY_LEFT_ALT, KEY_TAB, "Alt+Tab");
  } else if (cmd == "SHUT") {
    logEvent("KB", "Keyboard shutdown");

    // Ensure desktop is focused
    digitalWrite(kbLedPin, HIGH);
    Keyboard.press(KEY_LEFT_GUI);
    Keyboard.press('d');  // Win+D
    safeDelay(100);
    Keyboard.releaseAll();
    safeDelay(500);

    // Open shutdown dialog
    Keyboard.press(KEY_LEFT_ALT);
    Keyboard.press(KEY_F4);
    safeDelay(200);
    Keyboard.releaseAll();
    safeDelay(500);

    // Confirm shutdown
    Keyboard.press(KEY_RETURN);
    safeDelay(100);
    Keyboard.release(KEY_RETURN);
    digitalWrite(kbLedPin, LOW);
  } else if (cmd == "FULL") {
    logEvent("KB", "Keyboard fullscreen");
    pressCombo(KEY_LEFT_ALT, KEY_RETURN, "Alt+Enter");
  } else if (cmd == "DISP_PC") {
    switchDisplay(0, "Switching to PC screen only");
  } else if (cmd == "DISP_DUP") {
    switchDisplay(1, "Switching to Duplicate display");
  } else if (cmd == "DISP_EXT") {
    switchDisplay(2, "Switching to Extend display");
  } else if (cmd == "DISP_SEC") {
    switchDisplay(3, "Switching to Second screen only");
  } else if (cmd.startsWith("TYPE:")) {
    logEvent("KB", "Write to buffer: " + cmd.substring(5));
    Keyboard.print(cmd.substring(5));
  } else {
    logEvent("KB", "Unknown keyboard command: " + cmd);
  }
}

// Handles commands like PING, HID reset, etc.
void processSystemCommand(String cmd) {
  if (cmd == "PING") {
    logEvent("SYS", "Optimus is online");
    flashLed(relayLedPin, 1);
    flashLed(kbLedPin, 2);
  } else if (cmd == "KBRST") {
    logEvent("SYS", "Resetting HID");
    Keyboard.end();
    Keyboard.begin();
    flashLed(kbLedPin, 2);
  } else if (cmd == "DBG") {
    logEvent("SYS", "Toggling debug logs");
    debugState = !debugState;
  } else if (cmd == "LOCKOUT") {
    logEvent("SYS", "Toggling override lockout");
    lockoutState = !lockoutState;
    logEvent("SYS", "Override state: "+String(lockoutState));
  } else if (cmd == "WDT") {
    if (wdtEnable) {
      wdtEnable = 0;
      wdt_disable();
      logEvent("SYS", "Stop watchdog");
    } else {
        // Enable watchdog with 8-second timeout
        wdtEnable = 1;
        wdt_enable(WDTO_8S);
        logEvent("SYS", "Setup watchdog with 8 second timer");
    }
  } else if (cmd == "RST") {
    logEvent("SYS", "System going down for a reboot...");
    if (!wdtEnable) {
      wdt_enable(WDTO_8S);
    }
    // simply sleep for 9 seconds
    delay(9000);
  } else if (cmd == "RXRST") {
    Serial1.end();
    Serial1.begin(9600);
    logEvent("SYS", "Reset Serial");
  }
   else {
    logEvent("SYS", "Unknown system command: " + cmd);
  }
}

// Special commands like BIOS boot
void processSpecialCommand(String cmd) {
  if (cmd == "BIOS") {
    logEvent("SYS", "Booting to BIOS");
    processPowerCommand("PRESS");

    // For the next 10 seconds spam Delete key
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
      keyPress(KEY_DELETE, "Delete");
    }
  }
}

// Dispatches command to relevant handler
void processCommand(String cmd) {
  cmd.trim();
  logEvent("DBG", "Received command: " + cmd);

  if (cmd.startsWith("PWR_")) {
    processPowerCommand(cmd.substring(4));
  } else if (cmd.startsWith("KB_")) {
    processKeyboardCommand(cmd.substring(3));
  } else if (cmd.startsWith("SYS_")) {
    processSystemCommand(cmd.substring(4));
  } else if (cmd.startsWith("SPC_")) {
    processSpecialCommand(cmd.substring(4));
  } else {
    logEvent("SYS", "Unknown command: " + cmd);
  }
}

void setup() {
  // Initialize LED pins
  pinMode(relayLedPin, OUTPUT);
  pinMode(kbLedPin, OUTPUT);
  pinMode(overrideLed, OUTPUT);

  digitalWrite(relayLedPin, LOW);
  digitalWrite(kbLedPin, LOW);
  digitalWrite(overrideLed, LOW);

  // Initialize relay
  pinMode(relayPin, OUTPUT);
  pinMode(pwrOverride, INPUT_PULLUP);
  digitalWrite(relayPin, HIGH);

  // Initialize interfaces
  Serial1.begin(9600);
  Keyboard.begin();

  logEvent("SYS", "Optimus listening for commands...");
}

void loop() {
  int cmdLen = 0;
  if (wdtEnable) {
    wdt_reset();
    logEvent("DBG", "Reset watchdog timer");
  }
  if (!lockoutState && !digitalRead(pwrOverride)) {
    logEvent("SYS", "Overriding relay...");
    digitalWrite(overrideLed, HIGH);
    digitalWrite(relayLedPin, HIGH);
    digitalWrite(relayPin, LOW);
    while(!digitalRead(pwrOverride)) {
      safeDelay(50);
    }
    digitalWrite(overrideLed, LOW);
    digitalWrite(relayLedPin, LOW);
    digitalWrite(relayPin, HIGH);
    logEvent("SYS", "Override removed");
  }

  while (Serial1.available()) {
    if (cmdLen >= 255) {
      logEvent("DBG", "Command buffer overflow. Resetting.");
      cmdLen = 0;
      command = "";
      while (Serial1.available())
        Serial1.read();
    }
    char c = Serial1.read();
    if (c == '\n') {
      processCommand(command);
      command = "";
    } else {
      command += c;
      cmdLen++;
    }
  }
}
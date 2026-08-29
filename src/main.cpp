#include <Arduino.h>
#include "USB.h"
#include "USBHIDMouse.h"
#include "USBHIDKeyboard.h"

USBHIDMouse Mouse;
USBHIDKeyboard Keyboard;

const int pinVRX = 6;
const int pinVRY = 5;
const int pinSW  = 4;
const int pinKeyCtrlC = 1;
const int pinKeyCtrlV = 2;
const int pinKeyCtrlS = 3;
const int pinKeyCtrlX = 7;

const int DEADZONE = 50;
const int MAX_SPEED = 10;
const int SAMPLES = 4;
const unsigned long POLL_INTERVAL = 10;
const unsigned long DEBOUNCE_MS = 30;

int joyCenterX = 2048;
int joyCenterY = 2048;

unsigned long lastPoll = 0;
unsigned long lastBtnChange = 0;
bool lastBtnRaw = HIGH;
bool btnStable = HIGH;

unsigned long lastKeyChange = 0;
bool lastKeyRaw = HIGH;
bool keyStable = HIGH;

unsigned long lastKeyVChange = 0;
bool lastKeyVRaw = HIGH;
bool keyVStable = HIGH;

unsigned long lastKeySChange = 0;
bool lastKeySRaw = HIGH;
bool keySStable = HIGH;

unsigned long lastKeyXChange = 0;
bool lastKeyXRaw = HIGH;
bool keyXStable = HIGH;

unsigned long doubleClickTimer = 0;
int doubleClickState = 0; // 0=待機, 1=1回目クリック完了, 2=2回目クリック完了

unsigned long btnPressStart = 0;
bool longPressTriggered = false;

int readSmoothed(int pin) {
  long sum = 0;
  for(int i=0;i<SAMPLES;i++) sum += analogRead(pin);
  return sum / SAMPLES;
}

int applyDeadzoneAndScale(int delta) {
  if (abs(delta) <= DEADZONE) return 0;
  int sign = delta > 0 ? 1 : -1;
  int absDelta = abs(delta);
  int scaled = map(absDelta, DEADZONE, 2048, 0, MAX_SPEED);
  return sign * constrain(scaled, 0, MAX_SPEED);
}

void calibrateCenter() {
  long sumX = 0, sumY = 0;
  const int N = 200;
  for(int i=0;i<N;i++){
    sumX += analogRead(pinVRX);
    sumY += analogRead(pinVRY);
    delay(2);
  }
  joyCenterX = sumX / N;
  joyCenterY = sumY / N;
  Serial.printf("Calibrated center X=%d Y=%d\n", joyCenterX, joyCenterY);
}

void setup() {
  Serial.begin(115200);
  pinMode(pinSW, INPUT_PULLUP);
  pinMode(pinKeyCtrlC, INPUT_PULLUP);
  pinMode(pinKeyCtrlV, INPUT_PULLUP);
  pinMode(pinKeyCtrlS, INPUT_PULLUP);
  pinMode(pinKeyCtrlX, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetPinAttenuation(pinVRX, ADC_11db);
  analogSetPinAttenuation(pinVRY, ADC_11db);

  calibrateCenter();

  USB.begin();
  Mouse.begin();
  Keyboard.begin();
  Serial.println("USB HID Mouse+Keyboard ready");
}

void loop() {
  unsigned long now = millis();
  if (now - lastPoll < POLL_INTERVAL) return;
  lastPoll = now;

  int valX = readSmoothed(pinVRX);
  int valY = readSmoothed(pinVRY);

  int deltaX = valX - joyCenterX;
  int deltaY = valY - joyCenterY;

  int moveX = applyDeadzoneAndScale(deltaX);
  int moveY = applyDeadzoneAndScale(deltaY);

  if (moveX != 0 || moveY != 0) {
    Mouse.move(-moveX, -moveY);
  }

  bool btnRaw = digitalRead(pinSW) == LOW;
  if (btnRaw != lastBtnRaw) {
    lastBtnChange = now;
    lastBtnRaw = btnRaw;
  }
  if (now - lastBtnChange > DEBOUNCE_MS) {
    if (btnStable != btnRaw) {
      btnStable = btnRaw;
      if (btnStable) {
        // 押下開始
        btnPressStart = now;
        longPressTriggered = false;
        // シングルクリック
        Serial.println("Single click");
        Mouse.click(MOUSE_LEFT);
      } else {
        btnPressStart = 0;
        longPressTriggered = false;
      }
    }
  }
  
  // 長押し検出: 1秒以上押し続けている場合
  if (btnStable && btnPressStart > 0 && !longPressTriggered) {
    if (now - btnPressStart >= 1000) {
      longPressTriggered = true;
      // 長押し検出時に追加のダブルクリック（2回）
      Serial.println("Long press detected - additional double click (total 3 clicks)");
      Mouse.click(MOUSE_LEFT);
      Mouse.click(MOUSE_LEFT);
    }
  }

  // Ctrl+C スイッチ処理 (IO1, GND へ)
  bool keyRaw = digitalRead(pinKeyCtrlC) == LOW;
  if (keyRaw != lastKeyRaw) {
    lastKeyChange = now;
    lastKeyRaw = keyRaw;
  }
  if (now - lastKeyChange > DEBOUNCE_MS) {
    if (keyStable != keyRaw) {
      keyStable = keyRaw;
      if (keyStable) {
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press('c');
        Serial.println("Ctrl+C pressed");
      } else {
        Keyboard.releaseAll();
        Serial.println("Ctrl+C released");
      }
    }
  }

  // Ctrl+V スイッチ処理 (IO2, GND へ)
  bool keyVRaw = digitalRead(pinKeyCtrlV) == LOW;
  if (keyVRaw != lastKeyVRaw) {
    lastKeyVChange = now;
    lastKeyVRaw = keyVRaw;
  }
  if (now - lastKeyVChange > DEBOUNCE_MS) {
    if (keyVStable != keyVRaw) {
      keyVStable = keyVRaw;
      if (keyVStable) {
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press('v');
        Serial.println("Ctrl+V pressed");
      } else {
        Keyboard.releaseAll();
        Serial.println("Ctrl+V released");
      }
    }
  }

  // Ctrl+S スイッチ処理 (IO3, GND へ)
  bool keySRaw = digitalRead(pinKeyCtrlS) == LOW;
  if (keySRaw != lastKeySRaw) {
    lastKeySChange = now;
    lastKeySRaw = keySRaw;
  }
  if (now - lastKeySChange > DEBOUNCE_MS) {
    if (keySStable != keySRaw) {
      keySStable = keySRaw;
      if (keySStable) {
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press('s');
        Serial.println("Ctrl+S pressed");
      } else {
        Keyboard.releaseAll();
        Serial.println("Ctrl+S released");
      }
    }
  }

  // Ctrl+X スイッチ処理 (IO7, GND へ)
  bool keyXRaw = digitalRead(pinKeyCtrlX) == LOW;
  if (keyXRaw != lastKeyXRaw) {
    lastKeyXChange = now;
    lastKeyXRaw = keyXRaw;
  }
  if (now - lastKeyXChange > DEBOUNCE_MS) {
    if (keyXStable != keyXRaw) {
      keyXStable = keyXRaw;
      if (keyXStable) {
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press('x');
        Serial.println("Ctrl+X pressed");
      } else {
        Keyboard.releaseAll();
        Serial.println("Ctrl+X released");
      }
    }
  }
}

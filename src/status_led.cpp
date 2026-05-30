#include "status_led.h"

namespace {
#if defined(BOARD_SEEED_XIAO_ESP32C3)
constexpr int kStatusLedPin = 20;
constexpr bool kUseNeoPixel = false;
constexpr bool kHasStatusLed = true;
#elif defined(BOARD_ADAFRUIT_QTPY_ESP32S3) && defined(PIN_NEOPIXEL)
constexpr bool kUseNeoPixel = true;
constexpr bool kHasStatusLed = true;
#elif defined(LED_BUILTIN)
constexpr int kStatusLedPin = LED_BUILTIN;
constexpr bool kUseNeoPixel = false;
constexpr bool kHasStatusLed = true;
#else
constexpr int kStatusLedPin = -1;
constexpr bool kUseNeoPixel = false;
constexpr bool kHasStatusLed = false;
#endif

bool gConnected = false;
uint32_t gLastUpdateMs = 0;
uint16_t gBrightness = 0;
int16_t gStep = 8;
constexpr uint32_t kBreathIntervalMs = 20;
constexpr uint8_t kPwmChannel = 0;
constexpr uint8_t kPwmResolution = 8;
constexpr uint32_t kPwmFrequencyHz = 5000;

void writeLed(uint8_t brightness) {
  if (!kHasStatusLed) {
    return;
  }

#if defined(BOARD_ADAFRUIT_QTPY_ESP32S3) && defined(PIN_NEOPIXEL)
  rgbLedWrite(RGB_BUILTIN, 0, 0, brightness);
#else
  ledcWrite(kStatusLedPin, brightness);
#endif
}
}

void initStatusLed() {
  if (!kHasStatusLed) {
    return;
  }

#if defined(BOARD_ADAFRUIT_QTPY_ESP32S3) && defined(NEOPIXEL_POWER)
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, NEOPIXEL_POWER_ON);
#endif

#if defined(BOARD_ADAFRUIT_QTPY_ESP32S3) && defined(PIN_NEOPIXEL)
  writeLed(0);
#else
  ledcAttach(kStatusLedPin, kPwmFrequencyHz, kPwmResolution);
  writeLed(0);
#endif
}

void setStatusLedConnected(bool connected) {
  gConnected = connected;
  if (!connected) {
    gBrightness = 0;
    gStep = 8;
    writeLed(0);
  }
}

void updateStatusLed() {
  if (!kHasStatusLed || !gConnected) {
    return;
  }

  uint32_t now = millis();
  if (now - gLastUpdateMs < kBreathIntervalMs) {
    return;
  }

  gLastUpdateMs = now;
  int32_t next = static_cast<int32_t>(gBrightness) + gStep;
  if (next >= 255) {
    next = 255;
    gStep = -gStep;
  } else if (next <= 0) {
    next = 0;
    gStep = -gStep;
  }

  gBrightness = static_cast<uint16_t>(next);
  writeLed(static_cast<uint8_t>(gBrightness));
}

void playSpeakerTriggerLedPattern() {
  if (!kHasStatusLed) {
    return;
  }

  bool wasConnected = gConnected;
  gConnected = false;
  for (uint8_t i = 0; i < 3; ++i) {
    writeLed(255);
    delay(120);
    writeLed(0);
    delay(120);
  }
  gConnected = wasConnected;
}

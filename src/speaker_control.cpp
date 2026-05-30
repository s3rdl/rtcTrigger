#include "speaker_control.h"

namespace {
void pulseRelay(uint8_t pin, uint32_t holdMs) {
  digitalWrite(pin, HIGH);
  delay(holdMs);
  digitalWrite(pin, LOW);
}
}

void initSpeakerControl() {
  pinMode(Pins::POWER_RELAY, OUTPUT);
  pinMode(Pins::MODE_RELAY, OUTPUT);
  digitalWrite(Pins::POWER_RELAY, LOW);
  digitalWrite(Pins::MODE_RELAY, LOW);
}

void pressPowerButton(uint32_t holdMs) {
  pulseRelay(Pins::POWER_RELAY, holdMs);
}

void pressModeButton(uint32_t holdMs) {
  pulseRelay(Pins::MODE_RELAY, holdMs);
}

void runSpeakerSequence(const AppSettings &settings) {
  pressPowerButton(settings.powerHoldMs);
  delay(settings.bootDelayMs);
  pressModeButton(settings.modePressMs);
}

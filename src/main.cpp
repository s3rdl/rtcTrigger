#include <Arduino.h>

#include "ble_control.h"
#include "rtc_control.h"
#include "settings.h"
#include "speaker_control.h"
#include "status_led.h"

namespace {
bool gRtcAvailable = false;
RTC_DATA_ATTR uint8_t gNextScheduledWakeKind = static_cast<uint8_t>(ScheduledWakeKind::None);
EventTelemetry gEventTelemetry{"none", "", 0};

bool recoveryRequested() {
  pinMode(Pins::RECOVERY_BUTTON, INPUT_PULLUP);
  delay(20);
  return digitalRead(Pins::RECOVERY_BUTTON) == LOW;
}

float readBatteryVoltage(const AppSettings &settings) {
  analogReadResolution(12);
  uint32_t rawTotal = 0;
  constexpr uint8_t kSampleCount = 8;
  for (uint8_t i = 0; i < kSampleCount; ++i) {
    rawTotal += analogRead(Pins::BATTERY_ADC);
    delay(2);
  }

  float rawAverage = static_cast<float>(rawTotal) / static_cast<float>(kSampleCount);
  float pinVoltage = (rawAverage / 4095.0f) * 3.3f;
  return scaleBatteryVoltage(settings, pinVoltage);
}

void persistAndReschedule(AppSettings &settings) {
  saveSettings(settings);
  clearRtcAlarmLatch();
  ScheduledWakeKind kind = ScheduledWakeKind::None;
  DateTime triggerAt;
  if (configureRtcAlarm(settings, kind, triggerAt)) {
    gNextScheduledWakeKind = static_cast<uint8_t>(kind);
  } else {
    gNextScheduledWakeKind = static_cast<uint8_t>(ScheduledWakeKind::None);
    disableRtcAlarm();
  }
}

void enterSleep(const AppSettings &settings) {
  clearRtcAlarmLatch();
  ScheduledWakeKind kind = ScheduledWakeKind::None;
  DateTime triggerAt;
  bool rtcWakeConfigured = configureRtcAlarm(settings, kind, triggerAt);
  if (rtcWakeConfigured) {
    gNextScheduledWakeKind = static_cast<uint8_t>(kind);
  } else {
    gNextScheduledWakeKind = static_cast<uint8_t>(ScheduledWakeKind::None);
  }
  configureWakeSources(settings, rtcWakeConfigured);
  Serial.flush();
  esp_deep_sleep_start();
}

void updateRuntimeClockState(BleRuntimeState &bleState, bool rtcAvailable) {
  if (!rtcAvailable) {
    bleState.rtcNow = "unavailable";
    bleState.nextTrigger = "unavailable";
    return;
  }

  DateTime now = getRtcNow();
  bleState.rtcNow = formatDateTime(now);

  DateTime nextTrigger;
  bleState.nextTrigger = nextTriggerForSettings(bleState.settings, now, nextTrigger)
                            ? formatDateTime(nextTrigger)
                            : "none";
}

void recordLastEvent(const char *eventName, const String &eventAt, bool incrementSpeakerRuns) {
  gEventTelemetry.lastEvent = String(eventName);
  gEventTelemetry.lastEventAt = eventAt;
  if (incrementSpeakerRuns) {
    ++gEventTelemetry.speakerRuns;
  }
  saveEventTelemetry(gEventTelemetry);
}

void refreshBleRuntimeState(BleRuntimeState &bleState) {
  bleState.batteryVoltage = readBatteryVoltage(bleState.settings);
  bleState.batteryPercent = batteryPercentFromVoltage(bleState.settings, bleState.batteryVoltage);
  updateRuntimeClockState(bleState, gRtcAvailable);
  bleState.lastEvent = gEventTelemetry.lastEvent;
  bleState.lastEventAt = gEventTelemetry.lastEventAt;
  bleState.speakerRuns = gEventTelemetry.speakerRuns;
}

void persistBleRuntimeSettings(BleRuntimeState &bleState) {
  saveSettings(bleState.settings);
  clearRtcAlarmLatch();
  ScheduledWakeKind kind = ScheduledWakeKind::None;
  DateTime triggerAt;
  if (configureRtcAlarm(bleState.settings, kind, triggerAt)) {
    gNextScheduledWakeKind = static_cast<uint8_t>(kind);
  } else {
    gNextScheduledWakeKind = static_cast<uint8_t>(ScheduledWakeKind::None);
    disableRtcAlarm();
  }
}
}

void setup() {
  Serial.begin(115200);
  delay(200);

  if (recoveryRequested()) {
    resetSettings();
    Serial.println("BOOT held at startup: settings reset to defaults");
  }

  initStatusLed();
  initSpeakerControl();

  AppSettings settings = loadSettings();
  gEventTelemetry = loadEventTelemetry();

  gRtcAvailable = initRtc();
  if (gRtcAvailable && rtcLostPower()) {
    syncRtcFromBuildTime();
  }

  DateTime now = gRtcAvailable ? getRtcNow() : DateTime(F(__DATE__), F(__TIME__));

  WakeTrigger wakeTrigger = detectWakeTrigger();
  if (wakeTrigger == WakeTrigger::RtcAlarm) {
    clearRtcAlarmLatch();
    if (static_cast<ScheduledWakeKind>(gNextScheduledWakeKind) == ScheduledWakeKind::Speaker) {
      Serial.println("RTC wake matched speaker event");
      if (gRtcAvailable && oneShotDue(settings, now)) {
        Serial.println("Consuming one-shot speaker trigger");
        settings.oneShotEnabled = false;
        saveSettings(settings);
      }
      Serial.println("Running speaker sequence now");
      recordLastEvent("speaker", formatDateTime(now), true);
      playSpeakerTriggerLedPattern();
      runSpeakerSequence(settings);
    } else {
      Serial.println("RTC wake matched non-speaker event");
      if (gRtcAvailable && bleOneShotDue(settings, now)) {
        Serial.println("Consuming one-shot BLE wake trigger");
        settings.bleOneShotEnabled = false;
        saveSettings(settings);
      }
      recordLastEvent("ble", formatDateTime(now), false);
    }
  }

  float batteryVoltage = readBatteryVoltage(settings);

  BleRuntimeState bleState{
      .settings = settings,
      .batteryVoltage = batteryVoltage,
      .batteryPercent = batteryPercentFromVoltage(settings, batteryVoltage),
      .rtcNow = "",
      .nextTrigger = "",
      .lastEvent = gEventTelemetry.lastEvent,
      .lastEventAt = gEventTelemetry.lastEventAt,
      .speakerRuns = gEventTelemetry.speakerRuns,
      .refreshRuntimeState = refreshBleRuntimeState,
      .persistSettings = persistBleRuntimeSettings,
      .authenticated = !settings.authEnabled,
      .triggerSequence = false,
      .settingsChanged = false,
      .statusRequested = false,
  };

  updateRuntimeClockState(bleState, gRtcAvailable);

  startBleWindow(bleState, settings.bleWindowMs);

  // BLE handlers may persist changes immediately, so always carry the
  // latest runtime settings forward before deciding what to do next.
  settings = bleState.settings;

  if (bleState.triggerSequence) {
    Serial.println("Running speaker sequence from direct BLE command");
    recordLastEvent("speaker", gRtcAvailable ? formatDateTime(getRtcNow()) : String("direct"), true);
    playSpeakerTriggerLedPattern();
    runSpeakerSequence(bleState.settings);
  }

  if (bleState.settingsChanged) {
    settings = bleState.settings;
    persistAndReschedule(settings);
  }

  enterSleep(settings);
}

void loop() {
}

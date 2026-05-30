#pragma once

#include <Arduino.h>

#include "settings.h"

struct BleRuntimeState;
using BleRuntimeRefreshFn = void (*)(BleRuntimeState &state);
using BleRuntimePersistFn = void (*)(BleRuntimeState &state);

struct BleRuntimeState {
  AppSettings settings;
  float batteryVoltage;
  uint8_t batteryPercent;
  String rtcNow;
  String nextTrigger;
  String lastEvent;
  String lastEventAt;
  uint32_t speakerRuns;
  BleRuntimeRefreshFn refreshRuntimeState;
  BleRuntimePersistFn persistSettings;
  bool authenticated;
  bool triggerSequence;
  bool settingsChanged;
  bool statusRequested;
};

void startBleWindow(BleRuntimeState &state, uint32_t windowMs);

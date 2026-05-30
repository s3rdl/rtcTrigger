#pragma once

#include <Arduino.h>

#include "settings.h"

void initSpeakerControl();
void pressPowerButton(uint32_t holdMs);
void pressModeButton(uint32_t holdMs);
void runSpeakerSequence(const AppSettings &settings);

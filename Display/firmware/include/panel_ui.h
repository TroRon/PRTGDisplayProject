#pragma once
#include "panel_model.h"
void uiBegin(void (*nextScenario)());
void uiUpdate(const panel::Snapshot& value);
void uiLoop(uint32_t now);

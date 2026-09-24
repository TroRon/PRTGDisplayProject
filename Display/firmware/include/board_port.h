#pragma once
#include "panel_model.h"
bool boardBegin();
void boardStatus(panel::Status status);
void boardDim(bool dimmed);
void boardTouch(int16_t& x, int16_t& y, bool& pressed);

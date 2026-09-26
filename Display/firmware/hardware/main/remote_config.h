#pragma once
#include "display_config.h"
namespace remoteconfig {
void begin();
bool managed();
void effectiveName(char* name,unsigned capacity);
void report(const displayconfig::Options& options);
bool hasPending();
bool pending(displayconfig::Command& command);
bool accept(const displayconfig::Command& command);
void failed(const char* code);
bool release();
bool restore(displayconfig::Command& command);
void service(const char* token,char* scratch,unsigned capacity);
}

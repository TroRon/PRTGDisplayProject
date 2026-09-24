#pragma once
#include "live.h"
namespace ota {
constexpr const char* Version="0.7.2";
constexpr unsigned Sequence=702;
constexpr const char* Board="waveshare-lcd5b-28151";
constexpr const char* Layout="eaglenet-ota-v1";
struct Config { unsigned version=1; bool direct=false; char url[384]="https://raw.githubusercontent.com/TroRon/PRTGDisplayProject/main/Display/ota/manifest.json"; };
struct Status {
 bool busy=false,available=false,pending=false,ready=false;
 unsigned progress=0,seconds=0;
 char offered[32]={};
 char message[160]="OTA wird vorbereitet";
};
void begin(bool networkReady);
void tick();
void config(Config&);
bool configure(const Config&);
void status(Status&);
bool check(const Config&);
bool install(const Config&);
void confirm();
void service(const live::Config&);
bool busy();
}

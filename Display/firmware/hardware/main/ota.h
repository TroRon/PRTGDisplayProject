#pragma once
#include "live.h"
namespace ota {
constexpr const char* Version="0.8.1";
constexpr unsigned Sequence=801;
constexpr unsigned MaxReleases=8;
constexpr const char* Board="waveshare-lcd5b-28151";
constexpr const char* Layout="eaglenet-ota-v1";
struct Config { unsigned version=1; bool direct=false; char url[384]="https://raw.githubusercontent.com/TroRon/PRTGDisplayProject/main/Display/ota/manifest.json"; };
struct Status {
 bool busy=false,available=false,pending=false,ready=false;
 unsigned progress=0,seconds=0;
 unsigned count=0,chosen=0,revision=0,targetSequence=0,targetSize=0;
 char versions[MaxReleases][32]={};
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
bool choose(unsigned index);
void confirm();
void service(const live::Config&);
bool busy();
}

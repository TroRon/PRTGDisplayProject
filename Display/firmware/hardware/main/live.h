#pragma once
#include "panel_model.h"
#include <cstring>
#include <cstdio>
namespace live {
enum class ApiState { Waiting, Reachable, TransportError, Denied, HttpError, InvalidData };
enum class SourceState { Unknown, Complete, Partial, Unavailable };
inline const char* apiText(ApiState state){
    switch(state){
        case ApiState::Reachable:return "erreichbar";
        case ApiState::TransportError:return "Verbindungsfehler";
        case ApiState::Denied:return "Zugriff abgelehnt";
        case ApiState::HttpError:return "HTTP-Fehler";
        case ApiState::InvalidData:return "Daten ungültig";
        default:return "wartet";
    }
}
inline const char* sourceText(SourceState state){
    switch(state){
        case SourceState::Complete:return "Daten vollständig";
        case SourceState::Partial:return "Daten unvollständig";
        case SourceState::Unavailable:return "Quellen ausgefallen";
        default:return "unbekannt";
    }
}
struct Config {
    unsigned version=1;
    char ssid[33]={};
    char password[64]={};
    char token[257]={};
    char ntp[128]="pool.ntp.org";
};
// Shared by touch UI, USB provisioning and NVS loader. Byte limits matter for Wi-Fi.
inline bool validate(const Config& c) {
    if(c.version!=1 || !memchr(c.ssid,0,sizeof(c.ssid)) || !memchr(c.password,0,sizeof(c.password)) ||
       !memchr(c.token,0,sizeof(c.token)) || !memchr(c.ntp,0,sizeof(c.ntp)))return false;
    if(!*c.ssid || strlen(c.password)<8 || (*c.token&&strlen(c.token)<16) || !*c.ntp)return false;
    for(const unsigned char* p=(const unsigned char*)c.token;*p;p++)if(*p<33||*p>126)return false;
    for(const unsigned char* p=(const unsigned char*)c.ntp;*p;p++)if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='.'||*p=='-'))return false;
    return true;
}
struct Status {
    ApiState api=ApiState::Waiting;
    SourceState source=SourceState::Unknown;
    bool retryPending=false;
    unsigned failedAttempts=0,roundtripMs=0;
    int disconnectReason=0;
    bool startupFailed=false;
    bool sourcesUnavailable=false;
    bool configured=false, wifi=false, clock=false;
    int http=0;
    char ip[20]="-";
    char message[128]="Einrichtung erforderlich";
};
bool begin();
void prepare();
bool read(panel::Snapshot&,bool& sourcesUnavailable);
void status(Status&);
void config(Config&);
bool save(const Config&);
constexpr unsigned MaxNetworks=20;
struct Network {char ssid[33]={};int signal=0;bool supported=false;};
struct Scan {bool busy=false;unsigned count=0,revision=0;Network networks[MaxNetworks];char message[96]="WLAN suchen oder Namen manuell eingeben";};
bool scanStart();
void scanStatus(Scan&);
struct Hotspot {bool active=false,pending=false,initialAdmin=false;unsigned seconds=0;char password[33]={};char message[112]="Einrichtungshotspot ausgeschaltet";};
bool hotspotStart();
bool needsInitialSetup();
void hotspotStop();
void hotspotStatus(Hotspot&);
void adminChanged();
}

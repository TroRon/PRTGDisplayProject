#pragma once
#include "panel_model.h"
#include <cstdio>
#include <cstring>
#include <new>

namespace presentation {
constexpr uint32_t DemoDelayMs=30000;
class Controller {
    bool waiting=false,active=false;
    uint32_t since=0;
public:
    bool update(const panel::Snapshot& live,bool sourcesUnavailable,uint32_t now) {
        // Warnings, critical systems and partial/missing measurements remain real.
        const bool unavailable=!live.apiAvailable||sourcesUnavailable;
        if(!unavailable){waiting=false;active=false;return false;}
        if(!waiting){waiting=true;since=now;}
        active=uint32_t(now-since)>=DemoDelayMs;
        return active;
    }
};
inline void generate(panel::Snapshot& out,uint32_t now) {
    // Reset in place: no 13 KiB temporary on the app task stack.
    out.~Snapshot();new(&out) panel::Snapshot;out.demo=true;out.hasUpdate=true;out.apiAvailable=true;
    out.complete=true;out.receivedAtMs=now;out.overall=panel::Status::Ok;
    const unsigned phase=(now/30000)%3;
    const char* names[]={"Demo-Server-A","Demo-Server-B","Demo-Backup","Demo-Docker","Demo-Switch","Demo-Webdienst"};
    const unsigned cats[]={0,0,1,2,3,4};
    out.entityCount=6;
    for(unsigned i=0;i<6;++i){
        auto& entity=out.entities[i];entity.category=cats[i];entity.status=panel::Status::Ok;
        snprintf(entity.name,sizeof(entity.name),"%s",names[i]);
        auto& cat=out.categories[cats[i]];cat.enabled=true;cat.status=panel::Status::Ok;++cat.total;++cat.good;
    }
    const unsigned cpu=18+(now/5000)%35;
    snprintf(out.entities[0].details,sizeof(out.entities[0].details),"CPU: %u,0 %%\nRAM: 42,5 %%\nSystemdisk: 38,0 %%\nTemperatur: 46,0 °C",cpu);
    snprintf(out.entities[1].details,sizeof(out.entities[1].details),"CPU: %u,0 %%\nRAM: 56,2 %%\nSystemdisk: 51,0 %%\nZFS-Zustand: OK",cpu+7);
    snprintf(out.entities[2].details,sizeof(out.entities[2].details),"Backups aktuell: 17\nBackups gesamt: 17\nBackups veraltet: 0\nDatastore: 67,7 %%");
    snprintf(out.entities[3].details,sizeof(out.entities[3].details),"Docker Engine: OK\nContainer aktiv: 24\nContainer gesamt: 24\nDocker-Disk: 49,0 %%");
    snprintf(out.entities[4].details,sizeof(out.entities[4].details),"Antwortzeit: 2,0 ms\nPorts aktiv: 18\nPorts gesamt: 24");
    snprintf(out.entities[5].details,sizeof(out.entities[5].details),"HTTP: 200\nAntwortzeit: 85 ms\nVerfügbarkeit: 99,9 %%");
    if(phase){
        const unsigned index=phase==1?3:1;auto& entity=out.entities[index];
        entity.status=phase==1?panel::Status::Warning:panel::Status::Critical;
        auto& cat=out.categories[entity.category];cat.status=entity.status;--cat.good;
        out.overall=entity.status;out.alertCount=1;
        snprintf(entity.details,sizeof(entity.details),phase==1?"Docker Engine: OK\nContainer aktiv: 23\nContainer gesamt: 24\nSimulierte Container-Warnung":"CPU: 96,0 %%\nRAM: 94,0 %%\nSimulierte Auslastungswarnung");
        snprintf(out.alerts,sizeof(out.alerts),"%s: %s\nSimulierter Testfall, kein echter Alarm\n\n",entity.name,panel::statusText(entity.status));
    }
    snprintf(out.message,sizeof(out.message),"DEMO · Fiktive Beispieldaten · Keine Live-Überwachung");
}
}

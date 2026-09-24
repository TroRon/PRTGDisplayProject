#pragma once
namespace webadmin {
struct Status { bool configured=false,running=false;char message[128]="Webzugang nicht eingerichtet"; };
void begin(bool networkReady);
void tick();
void status(Status&);
bool password(const char* value);
bool disable();
}

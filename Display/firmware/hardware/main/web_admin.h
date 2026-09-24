#pragma once
namespace webadmin {
struct Status { bool configured=false,running=false;char message[128]="Webzugang nicht eingerichtet"; };
void begin(bool networkReady);
void tick();
void status(Status&);
// Physical display only; never include this value in HTTP status or logs.
void initialPassword(char (&out)[13]);
bool password(const char* value);
bool disable();
}

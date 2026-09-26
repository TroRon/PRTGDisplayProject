#include "panel_model.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

namespace panel {
static const char* keys[] = {"compute", "backup", "docker", "network", "services"};
static const char* names[] = {"Infra", "Backup", "Docker", "Netzwerk", "Dienste"};
const char* categoryKey(unsigned i) { return i < CategoryCount ? keys[i] : ""; }
const char* categoryName(unsigned i) { return i < CategoryCount ? names[i] : ""; }
const char* statusText(Status s) {
  switch (s) {
    case Status::Ok: return "OK";
    case Status::Warning: return "Warnung";
    case Status::Critical: return "Kritisch";
    default: return "Unbekannt";
  }
}
static bool decode(const char* text, Status& out) {
  if (!text) return false;
  if (!strcmp(text, "ok")) out = Status::Ok;
  else if (!strcmp(text, "warning")) out = Status::Warning;
  else if (!strcmp(text, "critical")) out = Status::Critical;
  else if (!strcmp(text, "unknown")) out = Status::Unknown;
  else return false;
  return true;
}
Status combine(Status a, Status b) {
  if (a == Status::Critical || b == Status::Critical) return Status::Critical;
  if (a == Status::Unknown || b == Status::Unknown) return Status::Unknown;
  if (a == Status::Warning || b == Status::Warning) return Status::Warning;
  return Status::Ok;
}
unsigned severityRank(Status status) {
  return status == Status::Critical ? 0 : status == Status::Unknown ? 1 : status == Status::Warning ? 2 : 3;
}
static void trimUtf8(char* text) {
  size_t n = strlen(text);
  if (!n) return;
  size_t start = n-1;
  while (start && (static_cast<unsigned char>(text[start]) & 0xc0) == 0x80) --start;
  unsigned char c = text[start];
  size_t expected = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
  if (n-start < expected) text[start] = 0;
}
static bool append(char* target, size_t capacity, const char* format, ...) {
  size_t used = strlen(target);
  if (used >= capacity - 1) return false;
  va_list args; va_start(args, format);
  int written = vsnprintf(target + used, capacity - used, format, args);
  va_end(args);
  trimUtf8(target);
  return written >= 0 && size_t(written) < capacity-used;
}
void formatMetric(const char* key, double value, bool available, char* output, size_t capacity) {
  struct Metric { const char* key; const char* label; const char* unit; bool count; };
  static const Metric metrics[] = {
    {"cpu","CPU"," %",false}, {"ram","RAM"," %",false},
    {"root_disk","Systemdisk"," %",false}, {"disk","Docker-Disk"," %",false},
    {"temperature","Temperatur"," °C",false}, {"zfs_capacity","ZFS-Belegung"," %",false},
    {"usage","Datastore"," %",false}, {"daily_current","Backups aktuell","",true},
    {"daily_total","Backups gesamt","",true}, {"daily_old","Backups veraltet","",true},
    {"running","Container aktiv","",true}, {"total","Container gesamt","",true},
    {"engine","Docker Engine","",true}, {"zfs_health","ZFS-Zustand","",true}
  };
  const Metric* spec = nullptr;
  for (const auto& metric : metrics) if (!strcmp(key,metric.key)) { spec=&metric; break; }
  const char* label = spec ? spec->label : key;
  if (!available || !isfinite(value)) { snprintf(output,capacity,"%s: Keine Daten",label); return; }
  if (!strcmp(key,"engine") || !strcmp(key,"zfs_health")) {
    const char* state = value == 1 ? "OK" : value == 0 ? "Fehler" : "Unbekannt";
    snprintf(output,capacity,"%s: %s",label,state); return;
  }
  char number[48]; snprintf(number,sizeof(number),spec && spec->count ? "%.0f" : "%.1f",value);
  for (char* c=number; *c; ++c) if (*c=='.') *c=',';
  snprintf(output,capacity,"%s: %s%s",label,number,spec ? spec->unit : "");
  trimUtf8(output);
}
static const char* reasonText(const char* reason) {
  if (!strcmp(reason,"STALE")) return "Messwerte sind veraltet";
  if (!strcmp(reason,"NOT_CONFIGURED")) return "Noch nicht eingerichtet";
  if (!strcmp(reason,"CHANNEL_DATA_MISSING")) return "Messwerte fehlen";
  if (!strcmp(reason,"TIMESTAMP_MISSING")) return "Messzeit fehlt";
  if (!strcmp(reason,"TIMESTAMP_IN_FUTURE")) return "Messzeit liegt in der Zukunft";
  if (!strcmp(reason,"SENSOR_MISSING")) return "Sensor nicht gefunden";
  if (!strcmp(reason,"WAITING_FOR_DATA")) return "Warte auf Messwerte";
  if (!strcmp(reason,"PRTG_NOT_MONITORING")) return "Überwachung nicht aktiv";
  if (!strcmp(reason,"PRTG_STATUS")) return "Status von PRTG gemeldet";
  return "Datenquelle nicht erreichbar";
}
// UTC ISO timestamps emitted by the aggregator; no local time-zone dependency.
static int64_t timestamp(const char* text) {
  if (!text || strlen(text) != 24 || text[23] != 'Z') return -1;
  int y, m, d, h, min, sec, ms;
  if (sscanf(text, "%4d-%2d-%2dT%2d:%2d:%2d.%3dZ", &y,&m,&d,&h,&min,&sec,&ms) != 7) return -1;
  if (y < 2020 || y > 2100 || m < 1 || m > 12 || d < 1 || h < 0 || h > 23 || min < 0 || min > 59 || sec < 0 || sec > 59) return -1;
  const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (d > days[m-1] + (m == 2 && y%4 == 0 && (y%100 != 0 || y%400 == 0))) return -1;
  y -= m <= 2;
  const int era = y / 400;
  const unsigned year = y - era*400;
  const unsigned day = (153*(m + (m > 2 ? -3 : 9))+2)/5 + d-1;
  return ((int64_t)era*146097 + year*365 + year/4 - year/100 + day - 719468)*86400 + h*3600 + min*60 + sec;
}
bool parseHealth(const char* json, size_t length, bool allowDemo, int64_t utcNow, Snapshot& out) {
  out = Snapshot{};
  if (!json || length == 0 || length > 32768) return false;
  DynamicJsonDocument doc(49152);
  if (deserializeJson(doc, json, length, DeserializationOption::NestingLimit(12))) return false;
  if (!doc["schema_version"].is<int>() || doc["schema_version"].as<int>() != 1 || !doc["data_complete"].is<bool>()) return false;
  const char* mode = doc["mode"] | "";
  out.demo = !strcmp(mode, "demo");
  if ((out.demo && !allowDemo) || (!out.demo && strcmp(mode, "live"))) return false;
  Status reported;
  if (!decode(doc["overall"], reported)) return false;
  if (!out.demo) {
    int64_t generated = timestamp(doc["generated_at"]);
    int64_t updated = timestamp(doc["updated_at"]);
    if (utcNow < 1700000000 || generated < 0 || updated < 0 ||
        utcNow-generated > 60 || generated-utcNow > 60 || utcNow-updated > 180 || updated-utcNow > 60) return false;
  }
  out.sourceAgeSeconds = out.demo ? 0 : uint32_t(utcNow > timestamp(doc["updated_at"]) ? utcNow-timestamp(doc["updated_at"]) : 0);
  if (!doc["categories"].is<JsonObject>() || !doc["alerts"].is<JsonArray>()) return false;
  Status calculated = Status::Ok;
  unsigned active = 0;
  for (unsigned i=0; i<CategoryCount; ++i) {
    JsonObject c = doc["categories"][keys[i]];
    Category& category = out.categories[i];
    if (c.isNull() || !c["enabled"].is<bool>() || !decode(c["status"], category.status) || !c["entities"].is<JsonArray>()) return false;
    category.enabled = c["enabled"];
    if (!category.enabled) {
      category.status = Status::Unknown;
      snprintf(category.details, sizeof(category.details), "Noch nicht eingerichtet");
      continue;
    }
    ++active;
    JsonArray entities = c["entities"];
    if (entities.size() == 0 || entities.size() > 24) return false;
    Status entityHealth = Status::Ok;
    for (JsonObject e : entities) {
      Status state;
      if (!decode(e["status"], state) || !e["label"].is<const char*>()) return false;
      category.total++;
      if (state == Status::Ok) category.good++;
      entityHealth = combine(entityHealth, state);
      if (out.entityCount >= MaxEntities) return false;
      Entity& entity = out.entities[out.entityCount++];
      entity.category=i; entity.status=state;
      const char* id=e["id"] | "";
      // Do not truncate identities: a collision would mix favorites or history.
      if(strlen(id)<sizeof(entity.id))snprintf(entity.id,sizeof(entity.id),"%s",id);

      bool timestampsValid=true;int64_t oldest=0;
      snprintf(entity.name,sizeof(entity.name),"%s",e["label"].as<const char*>()); trimUtf8(entity.name);
      bool completeDetails = true;
      // Reserve room for an explicit truncation notice instead of silently losing systems.
      const size_t limit = sizeof(entity.details)-40;
      for (JsonObject s : e["sensors"].as<JsonArray>()) {
        const int64_t observed=timestamp(s["observed_at"]);
        if(observed<=0)timestampsValid=false;else if(!oldest||observed<oldest)oldest=observed;
        const char* reason = s["reason"] | "";
        if (*reason) completeDetails = append(entity.details,limit,"%s\n",reasonText(reason)) && completeDetails;
        for (JsonPair metric : s["metrics"].as<JsonObject>()) {
          char line[128];
          // Stable sensor/metric identity, never a rounded display string. Preserve
          // the first numeric metric even when stale: do not silently switch series.
          if(!*entity.trendKey&&metric.value().is<double>()){
            char key[128];int n=snprintf(key,sizeof(key),"%lu/%s",(unsigned long)(s["sensor_id"] | 0UL),metric.key().c_str());
            if(n>0&&size_t(n)<sizeof(entity.trendKey)){
              memcpy(entity.trendKey,key,size_t(n)+1);
              const double v=metric.value().as<double>();entity.trendValue=float(v);
              entity.trendObservedAt=observed>0&&observed<=UINT32_MAX?uint32_t(observed):0;
              entity.trendValid=!out.demo&&!*reason&&state!=Status::Unknown&&isfinite(v)&&isfinite(entity.trendValue)&&entity.trendObservedAt>0&&observed<=utcNow&&utcNow-observed<=180;
            }
          }
          formatMetric(metric.key().c_str(),metric.value().as<double>(),metric.value().is<double>(),line,sizeof(line));
          completeDetails = append(entity.details,limit,"%s\n",line) && completeDetails;
        }
      }
      entity.observedAt=timestampsValid&&oldest>0&&oldest<=UINT32_MAX?uint32_t(oldest):0;
      if (!completeDetails) append(entity.details,sizeof(entity.details),"\nWeitere Kennzahlen in PRTG");
      if (!*entity.details) snprintf(entity.details,sizeof(entity.details),"Keine Kennzahlen verfügbar");
    }
    if (entityHealth != category.status) return false;
    calculated = combine(calculated, category.status);
  }
  if (!active) calculated = Status::Unknown;
  if (reported != calculated) return false;
  out.complete = doc["data_complete"];
  if (!out.complete && calculated == Status::Ok) return false;
  for (JsonObject alert : doc["alerts"].as<JsonArray>()) {
    Status state = Status::Unknown; decode(alert["status"],state); ++out.alertCount;
    append(out.alerts, sizeof(out.alerts), "%s: %s\n%s\n\n", alert["label"] | "System", statusText(state), reasonText(alert["reason"] | "PRTG_STATUS"));
  }
  out.overall = calculated;
  snprintf(out.message, sizeof(out.message), "%s", out.demo ? "DEMO - synthetische Daten" : "Live-Daten vom Aggregator");
  return true;
}
bool State::receive(const char* json, size_t length, bool allowDemo, int64_t utcNow, uint32_t now) {
  Snapshot candidate;
  if (!parseHealth(json, length, allowDemo, utcNow, candidate)) { fail("Ungültige oder alte API-Daten"); return false; }
  value = candidate;
  value.apiAvailable=true; value.hasUpdate=true; value.receivedAtMs=now;
  received_ = now;
  valid_ = true;
  return true;
}
void State::fail(const char* message) {
  value.apiAvailable=false; value.overall=Status::Unknown; value.complete=false;
  value.alertCount=0; value.alerts[0]=0;
  for (unsigned i=0;i<CategoryCount;i++) {
    value.categories[i].status=Status::Unknown; value.categories[i].good=0;
    snprintf(value.categories[i].details,sizeof(value.categories[i].details),"%s",value.categories[i].enabled ? "Keine aktuellen Daten" : "Noch nicht eingerichtet");
  }
  for (unsigned i=0;i<value.entityCount;i++) {
    value.entities[i].status=Status::Unknown;
    snprintf(value.entities[i].details,sizeof(value.entities[i].details),"Keine aktuellen Daten");
  }
  snprintf(value.message, sizeof(value.message), "%s", message);
  valid_ = false;
}
bool State::expire(uint32_t now) {
  if (valid_ && uint32_t(now-received_) >= LinkTimeoutMs) { fail("Verbindung zum Aggregator verloren"); return true; }
  return false;
}
}

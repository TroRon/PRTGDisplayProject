#pragma once
#include <stdint.h>
#include <stddef.h>

namespace panel {
enum class Status { Unknown, Ok, Warning, Critical };
constexpr unsigned CategoryCount = 5;
constexpr unsigned MaxEntities = 24;
constexpr uint32_t LinkTimeoutMs = 45000;
struct Category {
  bool enabled = false;
  Status status = Status::Unknown;
  unsigned good = 0, total = 0;
  char details[96] = {};
};
struct Entity {
  unsigned category = 0;
  Status status = Status::Unknown;
  uint32_t observedAt = 0;
  char name[64] = {};
  char details[384] = {};
};
struct Snapshot {
  Status overall = Status::Unknown;
  bool demo = false;
  bool complete = false;
  bool apiAvailable = false, hasUpdate = false;
  uint32_t receivedAtMs = 0, sourceAgeSeconds = 0;
  unsigned entityCount = 0, alertCount = 0;
  Entity entities[MaxEntities];
  Category categories[CategoryCount];
  char alerts[1400] = {};
  char message[96] = "Warte auf Daten";
};
const char* statusText(Status status);
const char* categoryName(unsigned category);
const char* categoryKey(unsigned category);
Status combine(Status a, Status b);
unsigned severityRank(Status status);
void formatMetric(const char* key, double value, bool available, char* output, size_t capacity);
bool parseHealth(const char* json, size_t length, bool allowDemo, int64_t utcNow, Snapshot& result);
class State {
 public:
  Snapshot value;
  bool receive(const char* json, size_t length, bool allowDemo, int64_t utcNow, uint32_t now);
  void fail(const char* message);
  bool expire(uint32_t now);
 private:
  bool valid_ = false;
  uint32_t received_ = 0;
};
}

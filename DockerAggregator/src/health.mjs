import { categories } from './config.mjs';
import { numberOrNull, prtgStatus, rawTime } from './prtg.mjs';

export function overallStatus(statuses) {
  // A known failure stays visible, even if another source is unknown.
  for (const status of ['critical', 'unknown', 'warning']) if (statuses.includes(status)) return status;
  return statuses.length ? 'ok' : 'unknown';
}

export function sensorHealth(sensor, record, now, staleMs) {
  const base = { key: sensor.key, sensor_id: sensor.id, status: 'unknown', reason: null, observed_at: null, metrics: {} };
  if (!record || record.error) return { ...base, reason: record?.error || 'WAITING_FOR_DATA' };
  const observed = rawTime(record.row.lastcheck_raw);
  const reported = prtgStatus(record.row.status_raw);
  base.observed_at = observed === null ? null : new Date(observed).toISOString();
  base.reported_status = reported;
  if (observed === null) return { ...base, reason: 'TIMESTAMP_MISSING' };
  if (observed > now + 60000) return { ...base, reason: 'TIMESTAMP_IN_FUTURE' };
  if (now - observed > staleMs) return { ...base, reason: 'STALE' };
  let missing = Boolean(record.metrics_error);
  for (const [key, name] of Object.entries(sensor.metrics)) {
    const channel = record.channels.find(item => item.name === name);
    // Never parse localized display text or silently substitute zero.
    const value = numberOrNull(channel?.lastvalue_raw ?? channel?.lastvalue__raw);
    base.metrics[key] = value;
    if (value === null) missing = true;
  }
  base.status = reported === 'critical' ? 'critical' : missing ? 'unknown' : reported;
  base.reason = missing ? 'CHANNEL_DATA_MISSING' : reported === 'unknown' ? 'PRTG_NOT_MONITORING' : null;
  return base;
}

export function buildHealth(config, records, { now = Date.now(), mode = 'live', collectedAt = null } = {}) {
  const result = {
    schema_version: 1, mode, overall: 'unknown', generated_at: new Date(now).toISOString(),
    updated_at: collectedAt === null ? null : new Date(collectedAt).toISOString(),
    data_complete: false, categories: {}, alerts: []
  };
  for (const category of categories) {
    const entities = config.entities.filter(e => e.category === category).map(entity => {
      const sensors = entity.sensors.map(sensor => sensorHealth(sensor, records.get(`${entity.id}/${sensor.key}`), now, config.stale_seconds * 1000));
      const status = overallStatus(sensors.map(s => s.status));
      for (const sensor of sensors) {
        if (sensor.status !== 'ok') result.alerts.push({
          id: `${entity.id}/${sensor.key}`, category, entity: entity.id,
          label: entity.label, status: sensor.status, reason: sensor.reason || 'PRTG_STATUS', sensor: sensor.key
        });
      }
      return { id: entity.id, label: entity.label, status, sensors };
    });
    result.categories[category] = {
      enabled: entities.length > 0, status: overallStatus(entities.map(e => e.status)),
      reason: entities.length ? null : 'NOT_CONFIGURED', entities,
      entities_ok: entities.filter(e => e.status === 'ok').length, entities_total: entities.length
    };
  }
  const active = Object.values(result.categories).filter(c => c.enabled);
  result.overall = overallStatus(active.map(c => c.status));
  result.data_complete = active.length > 0 && !result.alerts.some(a => a.status === 'unknown' || a.reason === 'CHANNEL_DATA_MISSING');
  result.alerts.sort((a, b) => ['critical', 'unknown', 'warning'].indexOf(a.status) - ['critical', 'unknown', 'warning'].indexOf(b.status));
  return result;
}

export class Collector {
  constructor(config, collect, mode = 'live', clock = Date.now) {
    this.config = config; this.collect = collect; this.mode = mode; this.clock = clock;
    this.records = new Map(); this.collectedAt = null; this.running = false;
  }
  async poll() {
    if (this.running) return;
    this.running = true;
    try {
      const jobs = this.config.entities.flatMap(e => e.sensors.map(s => ({ key: `${e.id}/${s.key}`, sensor: s })));
      // Bounded concurrency, publish atomically after each poll.
      const next = new Map();
      let index = 0;
      await Promise.all(Array.from({ length: Math.min(3, jobs.length) }, async () => {
        while (index < jobs.length) {
          const job = jobs[index++];
          try { next.set(job.key, await this.collect(job.sensor)); }
          catch { next.set(job.key, { error: 'SOURCE_UNAVAILABLE' }); }
        }
      }));
      this.records = next;
      this.collectedAt = this.clock();
    } finally { this.running = false; }
  }
  snapshot() {
    return buildHealth(this.config, this.records, { now: this.clock(), mode: this.mode, collectedAt: this.collectedAt });
  }
}

import { readFileSync } from 'node:fs';

export const categories = ['compute', 'backup', 'docker', 'network', 'services'];

export function validateConfig(config) {
  if (!config || !Array.isArray(config.entities) || !config.entities.length) throw new Error('CONFIG_ENTITIES');
  for (const key of ['poll_seconds', 'stale_seconds', 'request_timeout_seconds']) {
    if (!Number.isInteger(config[key]) || config[key] < 1 || config[key] > 86400) throw new Error('CONFIG_TIMING');
  }
  if (config.stale_seconds < config.poll_seconds * 2) throw new Error('CONFIG_STALE_WINDOW');
  const url = new URL(config.prtg_url);
  if (url.protocol !== 'https:' || url.username || url.password || url.search || url.hash || url.pathname !== '/') {
    throw new Error('CONFIG_PRTG_HTTPS_ORIGIN');
  }
  const ids = new Set();
  const sensorIds = new Set();
  for (const entity of config.entities) {
    if (!/^[a-z0-9_-]+$/.test(entity.id) || ids.has(entity.id) || !categories.includes(entity.category) ||
        typeof entity.label !== 'string' || !entity.label.trim() || !Array.isArray(entity.sensors) || !entity.sensors.length) {
      throw new Error('CONFIG_ENTITY');
    }
    ids.add(entity.id);
    const keys = new Set();
    for (const sensor of entity.sensors) {
      if (!/^[a-z0-9_-]+$/.test(sensor.key) || keys.has(sensor.key)) throw new Error('CONFIG_SENSOR_KEY');
      keys.add(sensor.key);
      if (sensor.id !== null && (!Number.isInteger(sensor.id) || sensor.id < 1 || sensorIds.has(sensor.id))) {
        throw new Error('CONFIG_SENSOR_ID');
      }
      if (sensor.id !== null) sensorIds.add(sensor.id);
      if (!sensor.metrics || typeof sensor.metrics !== 'object' || Array.isArray(sensor.metrics) ||
          Object.values(sensor.metrics).some(v => typeof v !== 'string' || !v.trim())) throw new Error('CONFIG_METRICS');
    }
  }
  return config;
}

export function readConfig(path) {
  return validateConfig(JSON.parse(readFileSync(path, 'utf8').replace(/^\uFEFF/, '')));
}

export function readSecret(env, key, required = true) {
  const value = env[`${key}_FILE`] ? readFileSync(env[`${key}_FILE`], 'utf8').trim() : env[key]?.trim();
  if (required && !value) throw new Error(`MISSING_${key}`);
  return value || '';
}

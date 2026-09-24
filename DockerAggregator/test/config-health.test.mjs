import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readConfig, validateConfig } from '../src/config.mjs';
import { buildHealth, overallStatus } from '../src/health.mjs';

test('neutral example stays unknown until real sensors are configured', () => {
  const config = readConfig(new URL('../config/aggregator.example.json', import.meta.url));
  const health = buildHealth(config, new Map());
  assert.equal(health.overall, 'unknown');
  assert.equal(health.data_complete, false);
  assert.equal(health.categories.compute.entities[0].label, 'Mein Server');
  assert.equal(health.categories.docker.enabled, false);
  assert.equal(overallStatus(['unknown', 'critical']), 'critical');
  assert.equal(overallStatus(['ok', 'unknown']), 'unknown');
  assert.throws(() => validateConfig({ ...config, prtg_url: 'http://prtg.example.org' }));
  assert.throws(() => validateConfig({ ...config, entities: [...config.entities, ...config.entities] }));
});

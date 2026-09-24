import test from 'node:test';
import assert from 'node:assert/strict';
import { once } from 'node:events';
import { createApi, parseAllowedClients } from '../src/server.mjs';

test('backend requires the configured TCP peer and token; forwarded headers cannot bypass restriction', async t => {
  let reads = 0;
  const collector = { snapshot: () => { reads++; return { mode: 'test' }; } };
  for (const allowed of ['192.0.2.10', '127.0.0.1']) {
    const server = createApi(collector, 'test-token', parseAllowedClients(allowed));
    server.listen(0, '127.0.0.1');
    await once(server, 'listening');
    t.after(() => { server.closeAllConnections(); return new Promise(resolve => server.close(resolve)); });
    const url = `http://127.0.0.1:${server.address().port}`;
    assert.equal((await fetch(url + '/healthz')).status, 200);
    const response = await fetch(url + '/api/v1/health', { headers: {
      Authorization: 'Bearer test-token', 'X-Forwarded-For': '192.0.2.10', 'X-Real-IP': '192.0.2.10'
    } });
    assert.equal(response.status, allowed === '127.0.0.1' ? 200 : 403);
    assert.equal((await fetch(url + '/api/v1/health')).status, allowed === '127.0.0.1' ? 401 : 403);
  }
  assert.equal(reads, 1);
});

test('invalid or empty access lists fail closed', () => {
  assert.equal(parseAllowedClients(undefined), null);
  for (const value of ['', ' ', '192.0.2.10,', '0.0.0.0/0', '*', 'proxy']) {
    assert.throws(() => parseAllowedClients(value), /INVALID_ALLOWED_CLIENTS/);
  }
  assert.equal(parseAllowedClients('127.0.0.1,::1').size, 2);
});

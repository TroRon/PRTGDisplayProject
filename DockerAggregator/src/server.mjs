import { createServer } from 'node:http';
import { createHash, timingSafeEqual } from 'node:crypto';
import { isIP } from 'node:net';

export function parseAllowedClients(value) {
  if (value === undefined) return null;
  const clients = value.split(',').map(s => s.trim());
  if (!clients.length || clients.some(ip => !isIP(ip))) throw new Error('INVALID_ALLOWED_CLIENTS');
  return new Set(clients);
}

function authorized(header, token) {
  if (!token) return true;
  const hash = value => createHash('sha256').update(value).digest();
  return timingSafeEqual(hash(header || ''), hash(`Bearer ${token}`));
}

export function createApi(collector, token = '', allowedClients = null, firmware = null, admin = null) {
  return createServer((req, res) => {
    const send = (code, value) => {
      res.writeHead(code, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store', 'X-Content-Type-Options': 'nosniff' });
      res.end(JSON.stringify(value));
    };
    if (req.url === '/healthz' && req.method === 'GET') return send(200, { service: 'eaglenet-aggregator', status: 'running' });
    // Trust the TCP peer only; client-supplied forwarding headers cannot grant access.
    const peer = req.socket.remoteAddress?.replace(/^::ffff:/, '');
    if (allowedClients && !allowedClients.has(peer)) return send(403, { error: 'CLIENT_NOT_ALLOWED' });
    if (admin && (req.url === '/admin' || req.url === '/admin/' || req.url === '/admin.js' || req.url === '/admin.css' || req.url.startsWith('/api/admin/'))) {
      Promise.resolve(admin(req,res)).catch(() => { if(!res.headersSent)send(500,{error:'INTERNAL_ERROR'});else res.destroy(); });return;
    }
    if (firmware && (req.url.startsWith('/api/v1/firmware/') || req.url === '/updates' || req.url === '/updates.js')) {
      Promise.resolve(firmware(req,res)).catch(() => { if(!res.headersSent)send(500,{error:'INTERNAL_ERROR'});else res.destroy(); });return;
    }
    if (req.method !== 'GET') return send(405, { error: 'METHOD_NOT_ALLOWED' });
    if (req.url !== '/api/v1/health') return send(404, { error: 'NOT_FOUND' });
    if (!authorized(req.headers.authorization, token)) return send(401, { error: 'UNAUTHORIZED' });
    try { send(200, collector.snapshot()); }
    catch { send(500, { error: 'INTERNAL_ERROR' }); }
  });
}

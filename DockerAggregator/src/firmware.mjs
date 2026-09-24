import { createHash, verify, timingSafeEqual } from 'node:crypto';
import { readFile, writeFile, rename, mkdir, unlink } from 'node:fs/promises';
import { join } from 'node:path';
import { readFileSync } from 'node:fs';

const MAX = 0x400000 + 8196;
const publicKey = readFileSync(new URL('./ota-public.pem', import.meta.url));
const tokenOk = (header, token) => !!token && timingSafeEqual(createHash('sha256').update(header || '').digest(), createHash('sha256').update(`Bearer ${token}`).digest());
export function validatePackage(data, key = publicKey) {
  if (data.length < 5 || data.length > MAX) throw Error('SIZE');
  const n = data.readUInt32BE(0);
  if (!n || n > 8192 || 4 + n >= data.length) throw Error('HEADER');
  const envelope = data.subarray(4, 4 + n);
  const wrapper = JSON.parse(envelope);
  if (typeof wrapper.payload !== 'string' || typeof wrapper.signature !== 'string') throw Error('ENVELOPE');
  const payload = Buffer.from(wrapper.payload, 'base64'), signature = Buffer.from(wrapper.signature, 'base64');
  if (payload.length > 2047 || signature.length !== 256 || !verify('RSA-SHA256', payload, key, signature)) throw Error('SIGNATURE');
  const m = JSON.parse(payload);
  const parts=typeof m.version==='string'?m.version.split('.').map(Number):[];
  const image = data.subarray(4 + n);
  if (m.schema !== 1 || m.board !== 'waveshare-lcd5b-28151' || m.layout !== 'eaglenet-ota-v1' ||
      !/^[0-9]+\.[0-9]+\.[0-9]+$/.test(m.version) || m.version.length > 31 || !Number.isInteger(m.sequence) || m.sequence < 1 || m.sequence > 0xffffffff ||
      !Number.isInteger(m.size) || m.size < 1024 || m.size > 0x400000 || image.length !== m.size ||
      !/^[a-f0-9]{64}$/.test(m.sha256) || createHash('sha256').update(image).digest('hex') !== m.sha256) throw Error('METADATA');
  if(parts.length!==3||parts.join('.')!==m.version||parts[0]>429495||parts[1]>99||parts[2]>99||m.sequence!==parts[0]*10000+parts[1]*100+parts[2])throw Error('VERSION');
  if (image[0] !== 0xe9 || image.readUInt16LE(12) !== 9 || image.readUInt32LE(32) !== 0xabcd5432 ||
      image.subarray(48, 80).toString().split('\0')[0] !== m.version || image.subarray(80, 112).toString().split('\0')[0] !== 'eaglenet_lcd5b') throw Error('BOARD');
  return { envelope, image, manifest: m };
}

export function createFirmwareService({ directory, adminToken, panelToken, key = publicKey, log = () => {} }) {
  if (!directory || !adminToken || adminToken.length < 32 || !panelToken || adminToken === panelToken) throw Error('OTA_CONFIG');
  let uploading = false;
  const send = (res, status, value) => {res.writeHead(status, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store', 'X-Content-Type-Options': 'nosniff' });res.end(JSON.stringify(value));};
  return async (req, res) => {
    if (req.url === '/updates' && req.method === 'GET') {
      res.writeHead(200, {'Content-Type':'text/html; charset=utf-8','Cache-Control':'no-store','X-Content-Type-Options':'nosniff','Content-Security-Policy':"default-src 'none'; script-src 'self'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'"});
      res.end(await readFile(new URL('./updates.html', import.meta.url)));return;
    }
    if(req.url === '/updates.js' && req.method === 'GET'){
      res.writeHead(200,{'Content-Type':'text/javascript; charset=utf-8','Cache-Control':'no-store','X-Content-Type-Options':'nosniff'});res.end(await readFile(new URL('./updates.js',import.meta.url)));return;
    }
    if (req.url === '/api/v1/firmware/upload' && req.method === 'PUT') {
      if (!tokenOk(req.headers.authorization, adminToken)) return send(res,401,{error:'UNAUTHORIZED'});
      if (uploading) return send(res,409,{error:'UPLOAD_BUSY'});
      if (Number(req.headers['content-length']) > MAX) return send(res,413,{error:'TOO_LARGE'});
      uploading = true;
      const temporary = join(directory,'.upload.tmp');
      try {
        let size = 0;const chunks=[];
        for await (const chunk of req) {size+=chunk.length;if(size>MAX){send(res,413,{error:'TOO_LARGE'});req.resume();return;}chunks.push(chunk);}
        const packageData=Buffer.concat(chunks), parsed=validatePackage(packageData,key);
        await mkdir(directory,{recursive:true});
        // Refuse rollback/replacement of an already published sequence.
        let previous;
        try {previous=validatePackage(await readFile(join(directory,'current.eagleota')),key).manifest;} catch(e) {if(e.code!=='ENOENT')throw e;}
        if(previous && parsed.manifest.sequence<=previous.sequence) return send(res,409,{error:'NOT_NEWER'});
        await writeFile(temporary,packageData,{flag:'wx',mode:0o660});
        await rename(temporary,join(directory,'current.eagleota'));
        log('INFO',`OTA: Firmware ${parsed.manifest.version} geprüft und bereitgestellt; keine Installation ausgelöst.`);
        return send(res,200,{version:parsed.manifest.version,sequence:parsed.manifest.sequence});
      } catch {log('WARN','OTA: Upload abgelehnt oder Speicherung fehlgeschlagen.');return send(res,400,{error:'PACKAGE_REJECTED'});}
      finally {await unlink(temporary).catch(()=>{});uploading=false;}
    }
    if(req.method !== 'GET')return send(res,405,{error:'METHOD_NOT_ALLOWED'});
    if(!tokenOk(req.headers.authorization,panelToken))return send(res,401,{error:'UNAUTHORIZED'});
    const binary=/^\/api\/v1\/firmware\/([a-f0-9]{64})\.bin$/.exec(req.url);
    if(req.url!=='/api/v1/firmware/manifest.json'&&!binary)return send(res,404,{error:'NOT_FOUND'});
    try {
      const p=validatePackage(await readFile(join(directory,'current.eagleota')),key);
      if(binary&&binary[1]!==p.manifest.sha256)return send(res,404,{error:'RELEASE_CHANGED_CHECK_AGAIN'});
      const data=binary?p.image:p.envelope;
      res.writeHead(200,{'Content-Type':binary?'application/octet-stream':'application/json','Content-Length':data.length,'Cache-Control':'no-store','X-Content-Type-Options':'nosniff'});res.end(data);
    }catch {send(res,404,{error:'NO_VALID_FIRMWARE'});}
  };
}

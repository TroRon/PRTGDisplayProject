// Build only from complete packages whose signature, board and image hash verify.
import {readdir,readFile,writeFile,mkdir} from 'node:fs/promises';
import {fileURLToPath} from 'node:url';
import {join} from 'node:path';
import {validatePackage} from '../DockerAggregator/src/firmware.mjs';
const root=fileURLToPath(new URL('../Display/',import.meta.url)),list=[];
for(const dir of await readdir(join(root,'releases'),{withFileTypes:true})){
 if(!dir.isDirectory())continue;
 const folder=join(root,'releases',dir.name,'ota');
 let files;try{files=await readdir(folder);}catch(e){if(e.code==='ENOENT')continue;throw e;}
 for(const file of files.filter(n=>n.endsWith('.eagleota'))){
  const release=validatePackage(await readFile(join(folder,file)));
  const same=list.find(p=>p.manifest.sequence===release.manifest.sequence);
  if(same&&same.manifest.sha256!==release.manifest.sha256)throw Error('Conflicting signed version');
  if(!same)list.push(release);
 }
}
list.sort((a,b)=>b.manifest.sequence-a.manifest.sequence);
if(!list.length)throw Error('No signed releases');
const selected=list.slice(0,8),catalog=JSON.stringify({schema:1,releases:selected.map(p=>JSON.parse(p.envelope))});
if(Buffer.byteLength(catalog)>16384)throw Error('Catalog exceeds panel capacity');
const output=join(root,'ota');await mkdir(output,{recursive:true});
for(const release of selected)await writeFile(join(output,release.manifest.sha256+'.bin'),release.image);
await writeFile(join(output,'manifest.json'),selected[0].envelope);
await writeFile(join(output,'catalog.json'),catalog);
console.log('Verified versions: '+selected.map(p=>p.manifest.version).join(', '));

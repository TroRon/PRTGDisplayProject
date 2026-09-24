document.querySelector('#upload').addEventListener('submit', async event => {
  event.preventDefault();const form=event.target,output=document.querySelector('#status'),token=document.querySelector('#token'),file=document.querySelector('#file').files[0];
  if(!file||file.size>0x400000+8196){output.textContent='Paket fehlt oder ist zu gross.';return;}
  form.querySelector('button').disabled=true;output.textContent='Paket wird übertragen und geprüft …';
  try {
    const response=await fetch('/api/v1/firmware/upload',{method:'PUT',headers:{Authorization:'Bearer '+token.value,'Content-Type':'application/octet-stream'},body:file,credentials:'omit'});
    token.value='';const data=await response.json();
    output.textContent=response.ok?'Firmware '+data.version+' in der Versionsliste verfügbar. Am Display «Versionen prüfen» wählen.':response.status===401?'Zugriff abgelehnt. OTA-Administrator-Token prüfen.':data.error==='CATALOG_FULL'?'Versionsliste voll (maximal 8). Alte Pakete gemäss Anleitung archivieren.':data.error==='VERSION_CONFLICT'?'Diese Version existiert bereits mit anderem Inhalt. Neue Versionsnummer verwenden.':response.status===409?'Ein Upload läuft bereits. Kurz warten.':'Paket abgelehnt. Signatur, Board und Serverzugriff prüfen.';
  }catch {output.textContent='Übertragung fehlgeschlagen. Verbindung und Server prüfen.';}
  finally {token.value='';form.querySelector('button').disabled=false;}
});

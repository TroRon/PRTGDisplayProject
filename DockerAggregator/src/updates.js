document.querySelector('#upload').addEventListener('submit', async event => {
  event.preventDefault();const form=event.target,output=document.querySelector('#status'),token=document.querySelector('#token'),file=document.querySelector('#file').files[0];
  if(!file||file.size>0x400000+8196){output.textContent='Paket fehlt oder ist zu gross.';return;}
  form.querySelector('button').disabled=true;output.textContent='Paket wird übertragen und geprüft …';
  try {
    const response=await fetch('/api/v1/firmware/upload',{method:'PUT',headers:{Authorization:'Bearer '+token.value,'Content-Type':'application/octet-stream'},body:file,credentials:'omit'});
    token.value='';const data=await response.json();
    output.textContent=response.ok?'Firmware '+data.version+' bereitgestellt. Am Display «Update prüfen» wählen.':response.status===401?'Zugriff abgelehnt. OTA-Administrator-Token prüfen.':response.status===409?'Upload läuft bereits oder Paket ist nicht neuer.':'Paket abgelehnt. Signatur, Board und Serverzugriff prüfen.';
  }catch {output.textContent='Übertragung fehlgeschlagen. Verbindung und Server prüfen.';}
  finally {token.value='';form.querySelector('button').disabled=false;}
});

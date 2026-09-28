import {PulseClient,mergePulse} from './pulse.mjs';
import {Collector} from './health.mjs';
import {PrtgClient, demoCollect} from './prtg.mjs';
import {createPollReporter} from './logging.mjs';

export class MonitoringRuntime {
  constructor({config, prtgToken, pulseToken='', demo=false, log=()=>{}, clientFactory=(c,t)=>new PrtgClient(c,t), pulseFactory=(c,t)=>new PulseClient(c,t)}) {
    this.pulseFactory=pulseFactory;this.demo=demo;this.log=log;this.clientFactory=clientFactory;this.round=0;this.stopped=true;this.busy=false;
    this.update({config,prtgToken,pulseToken});
  }
  update({config,prtgToken,pulseToken}) {
    const client=this.demo?null:this.clientFactory(config,prtgToken);
    this.current={config,client,pulse:config.pulse?.enabled&&!this.demo?this.pulseFactory(config,pulseToken):null,pulseRecord:null,hasPrtgToken:!!prtgToken,collector:new Collector(config,this.demo?demoCollect:!prtgToken?async()=>({error:'NOT_CONFIGURED'}):s=>client.collect(s),this.demo?'demo':'live')};
    this.report=createPollReporter(this.log);
    // New config starts unknown. Late results from the old collector can never become current.
    if(!this.stopped){clearTimeout(this.timer);if(!this.busy)this.timer=setTimeout(()=>this.tick(),0);}
  }
  snapshot(){const c=this.current;return mergePulse(c.collector.snapshot(),c.config,c.pulseRecord);}
  async tick(){
    if(this.stopped||this.busy)return;
    this.busy=true;const active=this.current,started=performance.now(),round=++this.round;
    this.log('INFO',`Runde ${round}: ${this.demo?'Demo-Daten erzeugen':'Quellenabfrage gestartet'}.`);
    try{await Promise.all([active.collector.poll(),active.pulse?active.pulse.collect().then(r=>{active.pulseRecord=r;}):Promise.resolve().then(()=>{if(this.demo&&active.config.pulse?.enabled)active.pulseRecord={observed:Date.now(),received:Date.now(),alerts:[{id:'pulse-demo',label:'Demo Service',status:'warning',message:'Synthetische Warnung',since:new Date().toISOString(),acknowledged:false}]};})]);if(active===this.current){this.report(this.snapshot(),round,Math.round(performance.now()-started));if(active.config.pulse?.enabled)this.log(active.pulseRecord?.error?'WARN':'INFO',`Pulse: ${active.pulseRecord?.error||`${active.pulseRecord?.alerts.length??0} aktive Alarme`}.`);}}
    catch{this.log('ERROR',`Runde ${round}: Verarbeitung fehlgeschlagen; nächste Abfrage folgt.`);}
    finally{this.busy=false;if(!this.stopped)this.timer=setTimeout(()=>this.tick(),active===this.current?this.current.config.poll_seconds*1000:0);}
  }
  start(){this.stopped=false;void this.tick();}
  stop(){this.stopped=true;clearTimeout(this.timer);}
}

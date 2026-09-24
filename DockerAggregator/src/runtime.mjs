import {Collector} from './health.mjs';
import {PrtgClient, demoCollect} from './prtg.mjs';
import {createPollReporter} from './logging.mjs';

export class MonitoringRuntime {
  constructor({config, prtgToken, demo=false, log=()=>{}, clientFactory=(c,t)=>new PrtgClient(c,t)}) {
    this.demo=demo;this.log=log;this.clientFactory=clientFactory;this.round=0;this.stopped=true;this.busy=false;
    this.update({config,prtgToken});
  }
  update({config,prtgToken}) {
    const client=this.demo?null:this.clientFactory(config,prtgToken);
    this.current={config,client,hasPrtgToken:!!prtgToken,collector:new Collector(config,this.demo?demoCollect:!prtgToken?async()=>({error:'NOT_CONFIGURED'}):s=>client.collect(s),this.demo?'demo':'live')};
    this.report=createPollReporter(this.log);
    // New config starts unknown. Late results from the old collector can never become current.
    if(!this.stopped){clearTimeout(this.timer);if(!this.busy)this.timer=setTimeout(()=>this.tick(),0);}
  }
  snapshot(){return this.current.collector.snapshot();}
  async tick(){
    if(this.stopped||this.busy)return;
    this.busy=true;const active=this.current,started=performance.now(),round=++this.round;
    this.log('INFO',`Runde ${round}: ${this.demo?'Demo-Daten erzeugen':'PRTG-Abfrage gestartet'}.`);
    try{await active.collector.poll();if(active===this.current)this.report(this.snapshot(),round,Math.round(performance.now()-started));}
    catch{this.log('ERROR',`Runde ${round}: Verarbeitung fehlgeschlagen; nächste Abfrage folgt.`);}
    finally{this.busy=false;if(!this.stopped)this.timer=setTimeout(()=>this.tick(),active===this.current?this.current.config.poll_seconds*1000:0);}
  }
  start(){this.stopped=false;void this.tick();}
  stop(){this.stopped=true;clearTimeout(this.timer);}
}

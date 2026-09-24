#include "preferences.h"
#include "esp_crt_bundle.h"
#include "ota.h"
#include "live.h"
#include "Arduino.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_memory_utils.h"
#include "memory_diagnostics.h"
#include "startup_sequence.h"
#include "source_health.h"
#include "link_policy.h"
#include "web_admin.h"
#include "esp_random.h"
#include <new>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <atomic>
#include <ctime>

extern const char root_ca[] asm("_binary_root_ca_pem_start");
namespace live {
static const char* Tag="EAGLENET_NET";
static QueueHandle_t updates,commands;
static SemaphoreHandle_t guard;
static Config current;
static Status currentStatus;
static std::atomic<bool> connected{false},synchronised{false};
static panel::State* model;
static panel::Snapshot* parsed;
static std::atomic<int> disconnectReason{0};
static bool sourcesUnavailable=false;
static Scan networks;
static std::atomic<bool> scanRequested{false};
static Hotspot setup;
static std::atomic<unsigned> setupCommand{0};
static bool initialSetup=false;
bool needsInitialSetup(){Status s;status(s);return initialSetup&&!s.configured;}
static esp_netif_t* apNetif=nullptr;
static uint32_t setupAt=0;
struct PsramJsonAllocator {
    void* allocate(size_t size){return heap_caps_malloc(size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
    void deallocate(void* ptr){heap_caps_free(ptr);}
    void* reallocate(void* ptr,size_t size){return heap_caps_realloc(ptr,size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
};
static StaticSemaphore_t guardStorage;
static StaticQueue_t updateControl,commandControl;
static uint8_t* updateStorage;
static uint8_t commandStorage[sizeof(Config)];
static constexpr unsigned WorkerStackBytes=32768;
alignas(16) static StackType_t workerStack[WorkerStackBytes/sizeof(StackType_t)];
static StaticTask_t workerControl;
static std::atomic<bool> ready{false};
static bool attempted=false,loopOwned=false,wifiInitialised=false;
static esp_event_handler_instance_t wifiHandler=nullptr,ipHandler=nullptr;
static esp_netif_t* netif;
static bool nvsReady=false;
static char* body;
static char ntpServer[128]; // Stable storage owned by SNTP until it is deinitialised.
static void publish(const char* message,int code=0) {
    xSemaphoreTake(guard,portMAX_DELAY);
    const bool changed=strcmp(currentStatus.message,message)!=0;
    currentStatus.wifi=connected;currentStatus.clock=synchronised;
    currentStatus.http=code;
    snprintf(currentStatus.message,sizeof(currentStatus.message),"%s",message);
    esp_netif_ip_info_t ip={};
    if(connected&&esp_netif_get_ip_info(netif,&ip)==ESP_OK)
        snprintf(currentStatus.ip,sizeof(currentStatus.ip),IPSTR,IP2STR(&ip.ip));
    else snprintf(currentStatus.ip,sizeof(currentStatus.ip),"-");
    currentStatus.sourcesUnavailable=sourcesUnavailable;
    xQueueOverwrite(updates,&model->value);
    xSemaphoreGive(guard);
    if(changed)ESP_LOGI(Tag,"STATE %s",message);
}
static void fail(const char* text,int code=0,ApiState api=ApiState::TransportError){
    link_policy::unavailable(model->value,text);
    xSemaphoreTake(guard,portMAX_DELAY);currentStatus.api=api;currentStatus.source=SourceState::Unknown;xSemaphoreGive(guard);
    publish(text,code);
}
static void event(void*,esp_event_base_t base,int32_t id,void* data) {
    if(base==WIFI_EVENT&&id==WIFI_EVENT_STA_DISCONNECTED){connected=false;if(data)disconnectReason=((wifi_event_sta_disconnected_t*)data)->reason;}
    if(base==IP_EVENT&&id==IP_EVENT_STA_GOT_IP)connected=true;
}
void prepare(){if(!guard)guard=xSemaphoreCreateMutexStatic(&guardStorage);}
void status(Status& out){
    if(guard)xSemaphoreTake(guard,portMAX_DELAY);
    out=currentStatus;out.wifi=connected;out.clock=synchronised;out.disconnectReason=disconnectReason;
    if(guard)xSemaphoreGive(guard);
}
void config(Config& out){if(guard)xSemaphoreTake(guard,portMAX_DELAY);out=current;if(guard)xSemaphoreGive(guard);}
bool save(const Config& value){return ready&&!ota::busy()&&commands&&validate(value)&&xQueueSend(commands,&value,0)==pdTRUE;}
bool scanStart(){
    if(!ready||ota::busy())return false;
    xSemaphoreTake(guard,portMAX_DELAY);bool ok=!networks.busy;
    if(ok){networks.busy=true;networks.count=0;++networks.revision;snprintf(networks.message,sizeof(networks.message),"Suche nach 2.4-GHz-WLANs …");scanRequested=true;}
    xSemaphoreGive(guard);return ok;
}
void scanStatus(Scan& out){if(guard)xSemaphoreTake(guard,portMAX_DELAY);out=networks;if(guard)xSemaphoreGive(guard);}
void hotspotStatus(Hotspot& out){if(guard)xSemaphoreTake(guard,portMAX_DELAY);out=setup;if(guard)xSemaphoreGive(guard);}
void adminChanged(){if(guard)xSemaphoreTake(guard,portMAX_DELAY);setup.initialAdmin=false;if(guard)xSemaphoreGive(guard);}
bool hotspotStart(){
    if(!ready||connected||ota::busy())return false;
    xSemaphoreTake(guard,portMAX_DELAY);bool allowed=!setup.active&&!setup.pending;
    if(allowed)setup.pending=true;
    xSemaphoreGive(guard);if(!allowed)return false;
    char password[33];unsigned char random[16];esp_fill_random(random,sizeof(random));
    for(unsigned i=0;i<16;++i)snprintf(password+2*i,3,"%02x",random[i]);
    webadmin::Status web;webadmin::status(web);bool initial=!web.configured;
    if(initial&&!webadmin::password(password)){xSemaphoreTake(guard,portMAX_DELAY);setup.pending=false;snprintf(setup.message,sizeof(setup.message),"WebAdmin-Passwort konnte nicht gespeichert werden.");xSemaphoreGive(guard);memset(password,0,sizeof(password));return false;}
    xSemaphoreTake(guard,portMAX_DELAY);snprintf(setup.password,sizeof(setup.password),"%s",password);setup.initialAdmin=initial;snprintf(setup.message,sizeof(setup.message),"Einrichtungshotspot wird gestartet …");xSemaphoreGive(guard);
    memset(password,0,sizeof(password));memset(random,0,sizeof(random));setupCommand=1;return true;
}
void hotspotStop(){setupCommand=2;}
static void setupService(){
    unsigned command=setupCommand.exchange(0);Hotspot current;hotspotStatus(current);
    if(command==2&&!current.active){xSemaphoreTake(guard,portMAX_DELAY);setup.pending=false;memset(setup.password,0,sizeof(setup.password));snprintf(setup.message,sizeof(setup.message),"Einrichtungshotspot ausgeschaltet");xSemaphoreGive(guard);return;}
    if(command==1){
        esp_err_t result=ESP_OK;
        if(!apNetif){
            esp_netif_config_t config=ESP_NETIF_DEFAULT_WIFI_AP();apNetif=esp_netif_new(&config);
            if(!apNetif)result=ESP_ERR_NO_MEM;
            else if((result=esp_netif_attach_wifi_ap(apNetif))==ESP_OK)result=esp_wifi_set_default_wifi_ap_handlers();
            if(result!=ESP_OK&&apNetif){esp_netif_destroy_default_wifi(apNetif);apNetif=nullptr;}
        }
        if(result==ESP_OK){
            // IDF DHCP DNS option: advertise the AP itself, never the upstream resolver.
            esp_netif_dhcps_stop(apNetif);
            esp_netif_dns_info_t dns={};dns.ip.type=ESP_IPADDR_TYPE_V4;dns.ip.u_addr.ip4.addr=ESP_IP4TOADDR(192,168,4,1);
            uint8_t offer=2;
            auto dnsResult=esp_netif_set_dns_info(apNetif,ESP_NETIF_DNS_MAIN,&dns);
            if(dnsResult==ESP_OK)dnsResult=esp_netif_dhcps_option(apNetif,ESP_NETIF_OP_SET,ESP_NETIF_DOMAIN_NAME_SERVER,&offer,sizeof(offer));
            auto dhcpResult=esp_netif_dhcps_start(apNetif);
            if(dnsResult!=ESP_OK||dhcpResult!=ESP_OK)ESP_LOGW(Tag,"Setup DNS advertisement unavailable; use manual browser address");
            wifi_config_t config={};snprintf((char*)config.ap.ssid,sizeof(config.ap.ssid),"SetupPRTGDisplay");config.ap.ssid_len=strlen("SetupPRTGDisplay");
            snprintf((char*)config.ap.password,sizeof(config.ap.password),"%s",current.password);config.ap.authmode=WIFI_AUTH_WPA2_PSK;config.ap.max_connection=2;config.ap.channel=1;config.ap.pmf_cfg.capable=true;
            // Configure before starting the AP, never expose a temporary open default AP.
            esp_wifi_stop();connected=false;result=esp_wifi_set_mode(WIFI_MODE_APSTA);
            if(result==ESP_OK)result=esp_wifi_set_config(WIFI_IF_AP,&config);
            if(result==ESP_OK)result=esp_wifi_start();
            memset(&config,0,sizeof(config));
        }
        if(result!=ESP_OK){esp_wifi_stop();if(esp_wifi_set_mode(WIFI_MODE_STA)==ESP_OK)esp_wifi_start();}
        setupAt=millis();xSemaphoreTake(guard,portMAX_DELAY);setup.pending=false;setup.active=result==ESP_OK;setup.seconds=setup.active?600:0;
        snprintf(setup.message,sizeof(setup.message),setup.active?"SetupPRTGDisplay bereit · http://192.168.4.1":"Hotspot-Start fehlgeschlagen; WebAdmin-Passwort bleibt gespeichert.");xSemaphoreGive(guard);
        ESP_LOGI(Tag,"Setup hotspot start result=%s",esp_err_to_name(result));
    }
    hotspotStatus(current);
    if(current.active){
        unsigned elapsed=uint32_t(millis()-setupAt)/1000;
        if(command==2||connected||elapsed>=600){
            auto stopped=esp_wifi_set_mode(WIFI_MODE_STA);
            if(stopped!=ESP_OK){setupCommand=2;ESP_LOGW(Tag,"Setup hotspot stop failed; retry pending");return;}
            xSemaphoreTake(guard,portMAX_DELAY);setup.active=false;setup.seconds=0;memset(setup.password,0,sizeof(setup.password));snprintf(setup.message,sizeof(setup.message),connected?"WLAN verbunden; Hotspot ausgeschaltet. Neue IP am Display prüfen.":"Einrichtungshotspot ausgeschaltet");xSemaphoreGive(guard);
            ESP_LOGI(Tag,"Setup hotspot stopped");
        }else{xSemaphoreTake(guard,portMAX_DELAY);setup.seconds=600-elapsed;xSemaphoreGive(guard);}
    }
    memset(&current,0,sizeof(current));
}
static void scanNetworks(bool running,bool configured){
    Scan result;result.revision=0;
    esp_err_t error=ESP_ERR_INVALID_STATE;
    if(running){
        // Connecting and explicit scans compete in the driver. Pause reconnect only when offline.
        if(!connected)esp_wifi_disconnect();
        wifi_scan_config_t cfg={};cfg.show_hidden=false;cfg.scan_time.active.max=120;
        error=esp_wifi_scan_start(&cfg,true);
        if(error==ESP_OK){
            auto* records=(wifi_ap_record_t*)heap_caps_calloc(MaxNetworks,sizeof(wifi_ap_record_t),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
            if(records){
                uint16_t count=MaxNetworks;error=esp_wifi_scan_get_ap_records(&count,records);
                if(error==ESP_OK)for(unsigned i=0;i<count;++i){
                    records[i].ssid[32]=0;const char* name=(const char*)records[i].ssid;bool safe=*name;
                    for(const unsigned char* c=records[i].ssid;*c;++c)if(*c<32||*c==127)safe=false;
                    bool duplicate=false;for(unsigned j=0;j<result.count;++j)if(!strcmp(result.networks[j].ssid,name))duplicate=true;
                    if(!safe||duplicate)continue;
                    auto& n=result.networks[result.count++];snprintf(n.ssid,sizeof(n.ssid),"%s",name);n.signal=records[i].rssi;
                    n.supported=records[i].authmode==WIFI_AUTH_WPA2_PSK||records[i].authmode==WIFI_AUTH_WPA_WPA2_PSK||records[i].authmode==WIFI_AUTH_WPA3_PSK||records[i].authmode==WIFI_AUTH_WPA2_WPA3_PSK;
                }
                heap_caps_free(records);
            }else error=ESP_ERR_NO_MEM;
            esp_wifi_clear_ap_list();
        }
        if(configured&&!connected)esp_wifi_connect();
    }
    snprintf(result.message,sizeof(result.message),error==ESP_OK?"%u WLANs gefunden · WPA2/WPA3 Personal":"Suche fehlgeschlagen; erneut versuchen oder manuell eingeben",result.count);
    xSemaphoreTake(guard,portMAX_DELAY);result.revision=networks.revision+1;networks=result;xSemaphoreGive(guard);
    ESP_LOGI(Tag,"WLAN scan done: result=%s count=%u",esp_err_to_name(error),result.count);
}
bool read(panel::Snapshot& value,bool& unavailable){
    if(!ready||!updates)return false;
    xSemaphoreTake(guard,portMAX_DELAY);
    const bool received=xQueueReceive(updates,&value,0)==pdTRUE;
    if(received)unavailable=currentStatus.sourcesUnavailable;
    xSemaphoreGive(guard);return received;
}
static bool persist(const Config& value) {
    if(!nvsReady)return false;
    nvs_handle_t handle;
    if(nvs_open("eagle-live",NVS_READWRITE,&handle)!=ESP_OK)return false;
    bool ok=nvs_set_blob(handle,"config",&value,sizeof(value))==ESP_OK&&nvs_commit(handle)==ESP_OK;
    nvs_close(handle);return ok;
}
static bool apply(const Config& value) {
    esp_wifi_stop();connected=false;synchronised=false;
    esp_netif_sntp_deinit();
    snprintf(ntpServer,sizeof(ntpServer),"%s",value.ntp);
    wifi_config_t wifi={};
    memcpy(wifi.sta.ssid,value.ssid,strlen(value.ssid));
    memcpy(wifi.sta.password,value.password,strlen(value.password));
    wifi.sta.threshold.authmode=WIFI_AUTH_WPA2_PSK;
    wifi.sta.pmf_cfg.capable=true;
    wifi.sta.sae_pwe_h2e=WPA3_SAE_PWE_BOTH;
    esp_err_t result=esp_wifi_set_config(WIFI_IF_STA,&wifi);
    memset(&wifi,0,sizeof(wifi));
    if(result!=ESP_OK)return false;
    logMemory("before_wifi_start");
    result=esp_wifi_start();logMemory("after_wifi_start");
    if(result!=ESP_OK){ESP_LOGE(Tag,"WiFi start failed: %s",esp_err_to_name(result));return false;}
    esp_sntp_config_t timeConfig=ESP_NETIF_SNTP_DEFAULT_CONFIG(ntpServer);
    timeConfig.sync_cb=[](struct timeval*){synchronised=true;};
    if(esp_netif_sntp_init(&timeConfig)!=ESP_OK)return false;
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_connect();return true;
}
static bool request(const Config& value,bool retry) {
    if(!*value.token){fail("WLAN bereit - Panel-Token in Einstellungen ergänzen",0,ApiState::Waiting);return false;}
    char endpoint[384];
    if(!preferences::endpoint("/api/v1/health",endpoint,sizeof(endpoint))){fail("Im Register Panel die Aggregator-Adresse eintragen",0,ApiState::Waiting);return false;}
    const int64_t started=esp_timer_get_time();
    esp_http_client_config_t options={};
    options.url=endpoint;options.crt_bundle_attach=esp_crt_bundle_attach;options.timeout_ms=6000;
    options.disable_auto_redirect=true;options.skip_cert_common_name_check=false;
    options.buffer_size=2048;options.buffer_size_tx=1024;
    bool retryable=false,valid=false;int code=0;esp_err_t err=ESP_OK;
    const char* stage="init";
    logMemory("before_https");
    esp_http_client_handle_t client=esp_http_client_init(&options);
    if(!client){err=ESP_ERR_NO_MEM;fail("HTTPS: Speicher nicht verfügbar");}
    else {
        char auth[sizeof(value.token)+8];snprintf(auth,sizeof(auth),"Bearer %s",value.token);
        stage="header";err=esp_http_client_set_header(client,"Authorization",auth);
        memset(auth,0,sizeof(auth));
        if(err==ESP_OK){stage="connect";err=esp_http_client_open(client,0);}
        if(err!=ESP_OK){fail("HTTPS/DNS fehlgeschlagen - Verbindung prüfen");retryable=true;}
        else {
            stage="response_headers";
            int64_t length=esp_http_client_fetch_headers(client);
            code=esp_http_client_get_status_code(client);
            if(length<0){err=ESP_FAIL;fail("HTTPS: Antwortheader unvollständig",code);retryable=true;}
            else if(code!=200){
                fail(code==401||code==403?"Panel-Zugang abgelehnt (Token/Rechte)":"Aggregator: HTTP-Fehler",code,
                    code==401||code==403?ApiState::Denied:ApiState::HttpError);
                retryable=link_policy::transientHttp(code);
            }else if(length>32768)fail("API-Antwort zu gross",code,ApiState::InvalidData);
            else {
                stage="body";size_t used=0;bool failed=false;
                while(!esp_http_client_is_complete_data_received(client)) {
                    if(used==32768||esp_timer_get_time()-started>12000000){failed=true;break;}
                    int received=esp_http_client_read(client,body+used,32768-used);
                    if(received<=0){failed=!esp_http_client_is_complete_data_received(client);break;}
                    used+=received;
                }
                body[used]=0;
                if(failed||!used){err=ESP_FAIL;fail("API-Antwort unvollständig",code,ApiState::InvalidData);retryable=used<32768;}
                else {
                    stage="validate";
                    // Parse into a PSRAM candidate: rejected JSON cannot destroy the last values.
                    valid=panel::parseHealth(body,used,false,time(nullptr),*parsed);
                    if(!valid)fail("Ungültige oder alte API-Daten",code,ApiState::InvalidData);
                    else {
                        model->value=*parsed;model->value.apiAvailable=true;model->value.hasUpdate=true;model->value.receivedAtMs=millis();
                        sourcesUnavailable=false;
                        BasicJsonDocument<PsramJsonAllocator> sourceDoc(49152);
                        const bool inspected=!deserializeJson(sourceDoc,body,used,DeserializationOption::NestingLimit(12));
                        if(inspected)sourcesUnavailable=allSourcesUnavailable(sourceDoc["categories"].as<JsonObject>());
                        xSemaphoreTake(guard,portMAX_DELAY);
                        currentStatus.api=ApiState::Reachable;
                        currentStatus.source=!inspected?SourceState::Unknown:sourcesUnavailable?SourceState::Unavailable:
                            model->value.complete?SourceState::Complete:SourceState::Partial;
                        xSemaphoreGive(guard);
                        publish(sourcesUnavailable?"Aggregator erreichbar - alle Datenquellen ausgefallen":
                            model->value.complete?"Live-Daten empfangen":"Live-Daten empfangen - PRTG-Daten unvollständig",code);
                        stage="complete";
                    }
                }
                memset(body,0,used);
            }
        }
        esp_http_client_close(client);esp_http_client_cleanup(client);
    }
    const unsigned elapsed=(unsigned)((esp_timer_get_time()-started)/1000);
    xSemaphoreTake(guard,portMAX_DELAY);
    currentStatus.roundtripMs=elapsed;
    if(valid)currentStatus.failedAttempts=0;else if(currentStatus.failedAttempts<0xffffffffu)++currentStatus.failedAttempts;
    const unsigned failures=currentStatus.failedAttempts;
    xSemaphoreGive(guard);
    logMemory("after_https");
    ESP_LOGI(Tag,"worker_stack_min_free_bytes=%u",(unsigned)uxTaskGetStackHighWaterMark(nullptr));
    ESP_LOGI(Tag,"Poll utc=%lld attempt=%s stage=%s err=%s HTTP=%d valid=%d sources_down=%d failures=%u duration_ms=%u wifi_reason=%d",
        (long long)time(nullptr),retry?"retry":"regular",stage,esp_err_to_name(err),code,valid,sourcesUnavailable,failures,elapsed,(int)disconnectReason.load());
    return retryable;
}
static void worker(void*) {
    Config active;config(active);bool configured=validate(active),running=false;
    if(configured)running=apply(active);else running=esp_wifi_start()==ESP_OK;
    uint32_t lastPoll=millis()-15000,lastConnect=millis();
    bool previousWifi=false;link_policy::RetryPolicy retryPolicy;
    for(;;) {
        Config candidate;
        if(xQueueReceive(commands,&candidate,0)==pdTRUE) {
            if(!persist(candidate)) {fail("Speichern fehlgeschlagen - bisherige Konfiguration bleibt");ESP_LOGW(Tag,"CONFIG_SAVE_FAILED");}
            else {
                active=candidate;
                xSemaphoreTake(guard,portMAX_DELAY);current=active;currentStatus.configured=true;xSemaphoreGive(guard);
                configured=true;running=apply(active);retryPolicy.reset();lastPoll=millis()-link_policy::PollMs;
                fail("Gespeichert - Verbindung wird aufgebaut",0,ApiState::Waiting);ESP_LOGI(Tag,"CONFIG_SAVED");
            }
        }
        memset(&candidate,0,sizeof(candidate));
        setupService();
        if(scanRequested.exchange(false))scanNetworks(running,configured);
        ota::service(active);
        const uint32_t now=millis();
        if(connected!=previousWifi){previousWifi=connected;ESP_LOGI(Tag,"WLAN %s reason=%d",previousWifi?"connected":"disconnected",(int)disconnectReason.load());retryPolicy.reset();lastPoll=now-link_policy::PollMs;}
        if(!configured)fail("Einstellungen öffnen: WLAN und Panel-Token erfassen",0,ApiState::Waiting);
        else if(!running)fail("Netzwerkstart fehlgeschlagen - Einstellungen erneut speichern");
        else if(!connected) {
            fail("WLAN nicht verbunden - SSID/Passwort/Signal prüfen");
            if(uint32_t(now-lastConnect)>=10000){esp_wifi_connect();lastConnect=now;}
        } else if(!synchronised||time(nullptr)<1735689600)fail("Warte auf NTP-Zeit - Zeitserver/Firewall prüfen",0,ApiState::Waiting);
        else if(uint32_t(now-lastPoll)>=retryPolicy.delay){
            const bool transient=request(active,retryPolicy.pending);retryPolicy.completed(transient);lastPoll=millis();
        }
        if(model->value.apiAvailable&&uint32_t(millis()-model->value.receivedAtMs)>=panel::LinkTimeoutMs)
            fail("Keine aktuellen Aggregator-Daten");
        if(!running||!connected||!synchronised)retryPolicy.reset();
        xSemaphoreTake(guard,portMAX_DELAY);currentStatus.retryPending=retryPolicy.pending;xSemaphoreGive(guard);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
static bool checked(esp_err_t result,const char* stage) {
    if(result==ESP_OK)return true;
    ESP_LOGE(Tag,"Startup stage=%s error=%s",stage,esp_err_to_name(result));return false;
}
static bool buffers() {
    body=(char*)heap_caps_malloc(32769,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    void* stateMemory=heap_caps_malloc(sizeof(panel::State),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(stateMemory)model=new(stateMemory) panel::State;
    updateStorage=(uint8_t*)heap_caps_malloc(sizeof(panel::Snapshot),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    void* candidate=heap_caps_malloc(sizeof(panel::Snapshot),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(candidate)parsed=new(candidate) panel::Snapshot;
    if(!body||!model||!parsed||!updateStorage)return false;
    updates=xQueueCreateStatic(1,sizeof(panel::Snapshot),updateStorage,&updateControl);
    commands=xQueueCreateStatic(1,sizeof(Config),commandStorage,&commandControl);
    ESP_LOGI(Tag,"Network buffers PSRAM: body=32769 state=%u queue=%u; worker stack INTERNAL static=%u",
        (unsigned)sizeof(panel::State),(unsigned)sizeof(panel::Snapshot),WorkerStackBytes);
    ESP_LOGI(Tag,"Network placement: body_psram=%d state_psram=%d queue_psram=%d candidate_psram=%d stack_internal=%d",
        esp_ptr_external_ram(body),esp_ptr_external_ram(model),esp_ptr_external_ram(updateStorage),esp_ptr_external_ram(parsed),esp_ptr_internal(workerStack));
    return updates&&commands&&esp_ptr_external_ram(body)&&esp_ptr_external_ram(model)&&esp_ptr_external_ram(updateStorage)&&esp_ptr_external_ram(parsed)&&esp_ptr_internal(workerStack);
}
static void freeBuffers() {
    if(updates){vQueueDelete(updates);updates=nullptr;}
    if(commands){vQueueDelete(commands);commands=nullptr;}
    heap_caps_free(updateStorage);updateStorage=nullptr;
    if(model){model->~State();heap_caps_free(model);model=nullptr;}
    if(parsed){parsed->~Snapshot();heap_caps_free(parsed);parsed=nullptr;}
    heap_caps_free(body);body=nullptr;
}
bool begin() {
    if(attempted)return ready;
    attempted=true;prepare();
    logMemory("before_network_resources");
    // No default-wifi convenience helper: it asserts on allocation failure.
    const startup::Step steps[]={
        {"buffers",[]{return guard&&buffers();},freeBuffers},
        {"netif_init",[]{return checked(esp_netif_init(),"netif_init");},nullptr},
        {"event_loop",[]{auto err=esp_event_loop_create_default();loopOwned=err==ESP_OK;return err==ESP_ERR_INVALID_STATE||checked(err,"event_loop");},
            []{if(loopOwned){esp_event_loop_delete_default();loopOwned=false;}}},
        {"station",[]{esp_netif_config_t cfg=ESP_NETIF_DEFAULT_WIFI_STA();netif=esp_netif_new(&cfg);return netif!=nullptr;},
            []{if(netif){esp_netif_destroy_default_wifi(netif);netif=nullptr;}}},
        {"attach",[]{return checked(esp_netif_attach_wifi_station(netif),"attach");},nullptr},
        {"station_handlers",[]{return checked(esp_wifi_set_default_wifi_sta_handlers(),"station_handlers");},nullptr},
        {"wifi_init",[]{
            logMemory("before_wifi_init");
            wifi_init_config_t wifi=WIFI_INIT_CONFIG_DEFAULT();wifi.nvs_enable=0;
            wifiInitialised=checked(esp_wifi_init(&wifi),"wifi_init");logMemory("after_wifi_init");return wifiInitialised;
        },[]{if(wifiInitialised){esp_wifi_stop();esp_wifi_deinit();wifiInitialised=false;}}},
        {"wifi_config",[]{return checked(esp_wifi_set_storage(WIFI_STORAGE_RAM),"storage")&&checked(esp_wifi_set_mode(WIFI_MODE_STA),"mode");},nullptr},
        {"wifi_events",[]{return checked(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,event,nullptr,&wifiHandler),"wifi_events");},
            []{if(wifiHandler){esp_event_handler_instance_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,wifiHandler);wifiHandler=nullptr;}}},
        {"ip_events",[]{return checked(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,event,nullptr,&ipHandler),"ip_events");},
            []{if(ipHandler){esp_event_handler_instance_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP,ipHandler);ipHandler=nullptr;}}},
        {"worker",[]{
            logMemory("before_network_task");
            // Reserve at link time, not as a late contiguous heap allocation after WiFi.
            return xTaskCreateStatic(worker,"health-api",WorkerStackBytes,nullptr,2,workerStack,&workerControl)!=nullptr;
        },nullptr}
    };
    nvsReady=nvs_flash_init()==ESP_OK;
    preferences::begin();
    if(nvsReady){nvs_handle_t handle;auto opened=nvs_open("eagle-live",NVS_READONLY,&handle);initialSetup=opened==ESP_ERR_NVS_NOT_FOUND;if(opened==ESP_OK){
        Config loaded;size_t size=sizeof(loaded);
        auto loadedResult=nvs_get_blob(handle,"config",&loaded,&size);initialSetup=loadedResult==ESP_ERR_NVS_NOT_FOUND;
        if(loadedResult==ESP_OK&&size==sizeof(loaded)&&validate(loaded)){
            if(guard)xSemaphoreTake(guard,portMAX_DELAY);
            current=loaded;currentStatus.configured=true;
            if(guard)xSemaphoreGive(guard);
        }
        memset(&loaded,0,sizeof(loaded));nvs_close(handle);
    }}
    for(const char* tag:{"HTTP_CLIENT","esp-tls","transport_base","transport_ssl","mbedtls"})esp_log_level_set(tag,ESP_LOG_NONE);
    esp_log_level_set("wifi",ESP_LOG_WARN);
    const char* failed=startup::run(steps,sizeof(steps)/sizeof(steps[0]));
    if(failed){
        if(guard)xSemaphoreTake(guard,portMAX_DELAY);
        currentStatus.startupFailed=true;currentStatus.wifi=false;currentStatus.clock=false;
        snprintf(currentStatus.message,sizeof(currentStatus.message),"Netzwerk nicht verfügbar (%s) - Live-Daten unbekannt",failed);
        if(guard)xSemaphoreGive(guard);
        ESP_LOGE(Tag,"Startup failed at %s; rollback complete; UI remains active",failed);
        logMemory("network_failed");return false;
    }
    ready=true;logMemory("after_network_task");return true;
}
}

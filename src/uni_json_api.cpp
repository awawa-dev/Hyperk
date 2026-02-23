#include "uni_json_api.h"
#include "config.h"
#include "utils.h"
#include "leds.h"
#include "volatile_state.h"
#include "mdns_service.h"

#include <charconv>

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
#else
    #include <WiFi.h>
#endif

#define TPL_BODY \
    R"raw({"state":{)raw" \
    R"raw("on":#####,"bri":####,"mainseg":0,"lor":0,)raw" \
    R"raw("seg":[{"id":0,"fx":0,"pal":0,"sel":true,"cct":0,"o1":false,"o2":false,"o3":false,"si":0,"m12":0,"bri":255,"start":0,"stop":######,"on":#####,"col":[[####,####,####],[0,0,0],[0,0,0]]}],)raw" \
    R"raw("nl":{"on":false,"dur":60,"mode":1,"tbri":0},)raw" \
    R"raw("udpn":{"send":false,"recv":false}},)raw" \
    R"raw("info":{)raw" \
    R"raw("ver":"0.15.3","vid":2508020,"cn":"Hyperk","name":"###############","mac":"############","arch":"###########","uptime":############,"live":#####,"freeheap":########,)raw" \
    R"raw("leds":{"count":######,"maxseg":1,"lc":1,"seglc":[1],"cct":0,"wv":0,"maxpwr":0,"rgbw":#####},)raw" \
    R"raw("wifi":{"rssi":######,"signal":####,"channel":###},)raw" \
    R"raw("fs":{"u":16,"t":61,"pmt":0}},)raw" \
    R"raw("effects":["Solid"],"palettes":["Default"]})raw"

static const char JSON_TPL[] PROGMEM = TPL_BODY;
static constexpr char TPL_CONST[] = TPL_BODY;

constexpr int getTokenPos(const char* target, int start_at = 0) {
    int i = start_at;
    while (TPL_CONST[i] != '\0') {
        bool found = true; int j = 0;
        while (target[j] != '\0') {
            if (TPL_CONST[i + j] != target[j]) { found = false; break; }
            j++;
        }
        if (found) return i + j;
        i++;
    }
    return -1;
}

static constexpr int P_ON    = getTokenPos("\"on\":");
static constexpr int P_BRI   = getTokenPos("\"bri\":");
static constexpr int P_SEGON = getTokenPos("\"on\":", P_ON);
static constexpr int P_STOP  = getTokenPos("\"stop\":");
static constexpr int P_R     = getTokenPos("\"col\":[[");
static constexpr int P_G     = P_R + 5;
static constexpr int P_B     = P_G + 5;
static constexpr int P_NAME  = getTokenPos("\"name\":\"");
static constexpr int P_MAC   = getTokenPos("\"mac\":\"");
static constexpr int P_ARCH  = getTokenPos("\"arch\":\"");
static constexpr int P_UP    = getTokenPos("\"uptime\":");
static constexpr int P_CNT   = getTokenPos("\"count\":", P_UP);
static constexpr int P_RSSI  = getTokenPos("\"rssi\":");
static constexpr int P_SIG   = getTokenPos("\"signal\":");
static constexpr int P_CH    = getTokenPos("\"channel\":");
static constexpr int P_RGBW  = getTokenPos("\"rgbw\":");
static constexpr int P_LIVE  = getTokenPos("\"live\":");
static constexpr int P_FREEH = getTokenPos("\"freeheap\":");

void fWrite(char* b, int p, int l, int32_t vv) {    
    auto bufferEnd = b + p + l;
    auto [lastChar, ec] = std::to_chars(b + p, bufferEnd, vv);
    memset(lastChar, ' ', bufferEnd - lastChar);
}

void sWrite(char* jsonTemplate, int fieldIndex, unsigned int freeSpace, const char* input, bool quote = true) {
    unsigned int copyLen = std::min(strlen(input), freeSpace);
    char* target = jsonTemplate + fieldIndex;

    memcpy(target, input, copyLen);
    if (quote) {
        target[copyLen++] = '"';
        freeSpace++;
    }
    memset(&target[copyLen], ' ', freeSpace - copyLen);
}

void bWrite(char* jsonTemplate, int fieldIndex, bool value) {
    const char* input = (value) ? "true" : "false";
    sWrite(jsonTemplate, fieldIndex, 5, input, false);
}

void uniConfigJsonResponse(AsyncWebServerRequest *request);

void setupUniApiJsonHandler(AsyncWebServer &server) {
    server.on("/presets.json", HTTP_GET, [](AsyncWebServerRequest *request){  
        
        Log::debug("-------- /presets.json ---------");
        Log::debug("{\"0\":{\"n\":\"Default\"}}");        

        request->send(200, "application/json", "{\"0\":{\"n\":\"Default\"}}");
    });

    server.on("/json/state", HTTP_POST | HTTP_PUT, [](AsyncWebServerRequest *request) {},NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            
            Log::debug("-------- /json/state ---------");
            Log::debug("--- Body starts ---");
            Log::raw("Inbound JSON: ", reinterpret_cast<char*>(data), len);
            Log::debug("--- Body ends ---");            

            JsonDocument doc;
            deserializeJson(doc, data, len);

            if (doc["bri"].is<int>())
            {
                Volatile::updateBrightness(doc["bri"]);
                Log::debug("[UNIAPI] BRI: ", Volatile::state.brightness);
            }
            else
            {
                doc["bri"] = Volatile::state.brightness;
            }

            if (doc["seg"].is<JsonArray>())
            {
                if (JsonArray sg = doc["seg"]; sg.size() > 0 && sg[0].is<JsonObject>()) 
                {
                    JsonObject seg = sg[0];
                
                    if (seg["on"].is<bool>())
                    {           
                        Volatile::updatePowerOn(seg["on"]);

                        seg["id"] = 0;
                        seg["bri"] = 255;
                        seg["start"] = 0;
                        seg["stop"] = Leds::getLedsNumber();
                        
                        Log::debug("[UNIAPI] SEG ON: ", Volatile::state.on);
                    }

                    if (seg["col"].is<JsonArray>())
                    {
                        if (JsonArray colors = seg["col"]; colors.size() > 0 && colors[0].is<JsonArray>()) {
                            if (JsonArray singleColor = colors[0]; singleColor.size() > 2){
                                Volatile::updateStaticColor(singleColor[0], singleColor[1], singleColor[2]);
                                Log::debug("[UNIAPI] COLOR: {", Volatile::state.staticColor.red, ", ", Volatile::state.staticColor.green, ", ", Volatile::state.staticColor.blue, "}");
                            }
                        }
                    }
                }
            }

            if (doc["on"].is<bool>())
            {           
                Volatile::updatePowerOn(doc["on"]);

                JsonArray segArray = doc["seg"].is<JsonArray>() ? doc["seg"].as<JsonArray>() : doc["seg"].to<JsonArray>();
                JsonObject seg = segArray.size() > 0 ? segArray[0] : segArray.add<JsonObject>();
                seg["id"] = 0;
                seg["bri"] = 255;
                seg["start"] = 0;
                seg["stop"] = Leds::getLedsNumber();
                seg["on"] = doc["on"];

                Log::debug("[UNIAPI] ON: ", Volatile::state.on);
            }


            JsonObject udpn = doc["udpn"].to<JsonObject>();
            udpn["send"] = false;
            udpn["recv"] = false;
            JsonObject nl = doc["nl"].to<JsonObject>();
            nl["on"] = false;            
            doc["lor"] = 0;

            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
            
            Log::debug("Outbound response JSON: ", response.c_str());
    });

    server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request){
        Log::debug("-------- /json ---------");
        uniConfigJsonResponse(request);
    });
}

void uniConfigJsonResponse(AsyncWebServerRequest *request)
{
    const AppConfig& cfg = Config::cfg;
    char b[sizeof(JSON_TPL)];
    memcpy_P(b, JSON_TPL, sizeof(JSON_TPL));

    // Current state
    bWrite(b, P_ON, Volatile::state.on);
    bWrite(b, P_SEGON, Volatile::state.on);

    // Brightness/LED count
    int ledsCount = Leds::getLedsNumber();
    fWrite(b, P_BRI, 4, Volatile::state.brightness);
    fWrite(b, P_STOP, 6, ledsCount);
    fWrite(b, P_CNT, 6, ledsCount);

    // Current colors
    fWrite(b, P_R, 4, Volatile::state.staticColor.red);
    fWrite(b, P_G, 4, Volatile::state.staticColor.green);
    fWrite(b, P_B, 4, Volatile::state.staticColor.blue);

    // Live
    bWrite(b, P_LIVE, Volatile::state.live);

    // RGBW capabilities
    bWrite(b, P_RGBW, (cfg.led.type == LedType::SK6812));

    // Device Name
    sWrite(b, P_NAME, 15, cfg.deviceName.c_str());

    // MAC
    sWrite(b, P_MAC, 12, Mdns::getDeviceShortMacAddress().c_str());    

    // Architecture
    sWrite(b, P_ARCH, 11, getDeviceArch().c_str());

    #if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
        uint32_t freeHeap = rp2040.getFreeHeap();
    #else
        uint32_t freeHeap = ESP.getFreeHeap();
    #endif
    fWrite(b, P_FREEH, 8, freeHeap);

    fWrite(b, P_UP, 12, millis() / 1000);

    int32_t rssiVal = WiFi.RSSI();
    fWrite(b, P_SIG, 4, (rssiVal <= -100) ? 0 : (rssiVal >= -50) ? 100
                                                                 : 2 * (rssiVal + 100));
    fWrite(b, P_CH, 3, WiFi.channel());

    // RSSI
    fWrite(b, P_RSSI, 6, rssiVal);
    
    Log::raw("Outbound JSON: ", b, sizeof(b));

    request->send(200, "application/json", b);
}

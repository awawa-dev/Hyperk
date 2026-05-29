// File: include/uni_json_api_oss.h
#pragma once

#include <cstddef>

class IHttpRequest;
class IHttpResponse;

namespace UniJsonApi {
    void uniConfigJsonResponse(IHttpRequest* request, IHttpResponse* response);
    void fillUniConfigJsonResponse(IHttpResponse* response, char* b);
}

#define TPL_BODY \
    R"raw({"state":{)raw" \
    R"raw("on":#####,"bri":####,"mainseg":0,"lor":0,)raw" \
    R"raw("seg":[{"id":0,"fx":0,"pal":0,"sel":true,"cct":0,"o1":false,"o2":false,"o3":false,"si":0,"m12":0,"bri":255,"start":0,"stop":######,"on":#####,"col":[[####,####,####],[0,0,0],[0,0,0]]}],)raw" \
    R"raw("nl":{"on":false,"dur":60,"mode":1,"tbri":0},)raw" \
    R"raw("udpn":{"send":false,"recv":false}},)raw" \
    R"raw("info":{)raw" \
    R"raw("ver":"0.15.3","vid":2508020,"cn":"######################","name":"###############","mac":"############","arch":"###########","uptime":############,"live":#####,"freeheap":########,)raw" \
    R"raw("leds":{"count":######,"maxseg":1,"lc":1,"seglc":[1],"cct":0,"wv":0,"maxpwr":0,"rgbw":#####},)raw" \
    R"raw("wifi":{"rssi":######,"signal":####,"channel":###},)raw" \
    R"raw("fs":{"u":16,"t":61,"pmt":0}},)raw" \
    R"raw("effects":["Solid"],"palettes":["Default"]})raw"

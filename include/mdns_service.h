// File: include/mdns.h
#pragma once

namespace Mdns{
    String sanitizeMdnsService(String s);
    String getDeviceShortMacAddress();
    void startMDNS();
    void endMDNS();
};

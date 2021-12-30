

#ifndef PILZ_NET_H_
#define PILZ_NET_H_

#include "../sys/walltime.h"
#include <string>
#include <cinttypes>
#include <vector>

#include "pico/stdlib.h"

struct ap_data_t
{
    std::string bssid;
    uint32_t    strength;   // signal strength in dbm
    uint8_t    channel;    // channel
    std::string encryption; // encryption type 
    std::string ssid;       // broadcast ssid of ap
};

struct scan_data_t
{
    uint16_t                count;      // number of networks found
    std::vector<ap_data_t>  apData;     // access point data
};

class pilznet
{
public:
    pilznet() {}
    ~pilznet() {}
    void init(void);

    bool connect(const std::string& ap, const std::string& pw);
    bool isConnected(void) const                { return (connected); }

    bool update(void);

    const scan_data_t scan(void);

    const std::string getMac(void) const        { return (macAddr); }
    const std::string getIP(void) const         { return (ipAddr); }

    bool doNTP(const std::string& tz);
    bool isClockValid(void) const               { return (clockValid); }

private:
    bool connected;
    bool clockValid;
    std::string ipAddr;
    std::string macAddr;
    walltime wt;

    const std::string encryption2text(int thisType);
    const std::string mac2text(uint8_t* mac);
    const std::string status2text(int status); 
};

#endif // PILZ_NET_H_
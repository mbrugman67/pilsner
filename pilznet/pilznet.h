/********************************************************
 * pilznet.h
 ********************************************************
 * Wrapper class for Adafruit Airlift Wifi module.  Uses
 * modified Aruduino libraries from adafruit.com
 * December 2021, M.Brugman
 * 
 *******************************************************/
#ifndef PILZ_NET_H_
#define PILZ_NET_H_

#include "../sys/walltime.h"
#include <string>
#include <cinttypes>
#include <vector>

#include "pico/stdlib.h"

// Structure to hold the results of an access point 
// scan.  This is for one access point, there will
// be a vector of them after the scan
struct ap_data_t
{
    std::string bssid;
    uint32_t    strength;   // signal strength in dbm
    uint8_t    channel;     // channel
    std::string encryption; // encryption type 
    std::string ssid;       // broadcast ssid of ap
};

// The full scan result - the number of networks and
// the data for each AP.  (Is the number really necessary?
// should just be able to use apData.size())
// TODO - check that out
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

    // call this periodically - it will service the UDP server
    bool update(void);

    // scan for all access points the Airlift module can see
    const scan_data_t scan(void);

    const std::string getMac(void) const        { return (macAddr); }
    const std::string getIP(void) const         { return (ipAddr); }

    // Do an NTP time/date lookup
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
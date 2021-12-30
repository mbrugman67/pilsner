/********************************************************
 * walltime.h
 ********************************************************
 * handle real-time clock management; setting, reading,
 * timezone
 * 
 * NOTE: The assumption here is that the wifi module
 * is already set up and running.  This just sends
 * and receives a UDP packet to do the time thing
 * 
 * 8 April 2021, M.Brugman
 * 
 *******************************************************/

#ifndef WALLTIME_H_
#define WALLTIME_H_

#include <cstddef>
#include <string>
#include <ctime>

#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

#include "../af/Wifi.h"
#include "../af/WifiUdp.h"

#define NTP_PACKET_SIZE  48

class walltime
{
public:
    walltime();
    ~walltime() {}

    const std::string listZones();

    bool isTimeValid()          {return (timeValid); }

    bool setTimezone(const std::string& zoneName);
    bool doNTP();
    bool setArmClockTime();

    static std::string timeString();
    static std::string dateString();
    static std::string logTimeString();

private:
    struct zdata_t
    {
        char name[16];
        char desc[32];
    };

    static zdata_t zones[];

    WiFiUDP udp;
    IPAddress timeServer;
    
    bool timeValid;
    uint8_t packetBuffer[NTP_PACKET_SIZE];      // buffer to hold incoming and outgoing packets
    time_t unixEpochTime;
};

#endif // WALLTIME_H_
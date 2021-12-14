/********************************************************
 * walltime.cpp
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

#include <cstring>
#include <machine/endian.h>

#include "pico/stdlib.h"
#include "walltime.h"

#define LOCAL_PORT 2390

#define NTP_PORT                123
#define SEVENTY_YEARS           (uint32_t)2208988800
#define SECONDS_PER_DAY         (uint32_t)86400

// Load some timezones.  If yours isn't hear, add it now!
walltime::zdata_t walltime::zones[] = {{"ADST9AKDT",    "US Alaska"},
                   {"PST8PDT",      "US Pacific"},
                   {"PST8",         "US Pacific (no DST)"},
                   {"MST7MDT",      "US Mountain"},
                   {"MST7",         "US Mountain (no DST)"},
                   {"CST6CDT",      "US Central"},
                   {"CST6",         "US Central (no DST)"},
                   {"EST5EDT",      "US Eastern"},
                   {"EST5",         "US Easter (no DST)"},
                   {"GMT-",         "GMT (Zulu)"},
                   {"BST-1",        "British Standard"},
                   {"CET-1CEST",    "Central European"}
                   };

/********************************************************
 * Constructor
 ********************************************************
 * Set the IP address and init the RTC module.
 * 
 * Future version should use DNS to hit pool.ntp.org
 * to get a rotation of timeservers instead of hard-
 * coding it.
 *******************************************************/
walltime::walltime()
{
    timeServer.fromString("129.6.15.28");
    timeValid = false;
    rtc_init();
}

/********************************************************
 * listZones()
 ********************************************************
 * return a std string of all possible timezones.
 *******************************************************/
const std::string walltime::listZones()
{
    char out[sizeof(zones) + 64] = {0};
    size_t remain = sizeof(out);
    size_t count = sizeof(zones) / sizeof(zones[0]);

    for (size_t ii = 0; ii < count; ++ii)
    {
        char line[sizeof(zones[0]) + 4];
        snprintf(line, sizeof(zones[0]) + 2, "%s: %s\n", zones[ii].name, zones[ii].desc);
        strncat(out, line, remain);
        remain -= strlen(line);
    }

    return (std::string(out));
}

/********************************************************
 * setTimezone()
 ********************************************************
 * Set the timezone, based on one of the existing zones
 * in the list
 *******************************************************/
bool walltime::setTimezone(const std::string& zoneName)
{
    // Set the environment variable for the timezone
    // let gcc and the standard library do all the real
    // work for use
    setenv("TZ", zoneName.c_str(), 1);
    return (true);
}

/********************************************************
 * doNTP()
 ********************************************************
 * Send a UDP packet to a timeserver and parse the 
 * response.  Sets the Pico RTC module to the current
 * time in the current timezone
 * 
 * This is based on example code from the Pico SDK
 *******************************************************/
bool walltime::doNTP()
{
    timeValid = false;
    std::memset(packetBuffer, 0, NTP_PACKET_SIZE);

    // prep a UDP packet to send
    udp.begin(LOCAL_PORT);

    // Initialize values needed to form NTP request
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;            // Stratum, or type of clock
    packetBuffer[2] = 6;            // Polling Interval
    packetBuffer[3] = 0xEC;         // Peer Clock Precision

    // 8 bytes of zero for estimated error and drift rate

    // 4 bytes, reference clock identifier
    packetBuffer[12]  = 49;         // 1
    packetBuffer[13]  = 0x4E;       // N
    packetBuffer[14]  = 49;         // 1
    packetBuffer[15]  = 52;         // 4

    // Send the packet
    udp.beginPacket(timeServer, NTP_PORT);
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();

    // wait for the reponse
    sleep_ms(4000);

    if (udp.parsePacket()) 
    {
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

        //the timestamp starts at byte 40 of the received packet and is four bytes
        uint32_t netTime;
        memcpy(&netTime, &packetBuffer[40], 4);

        // get the number of seconds since 1/1/1900 in the right byte order
        uint32_t secsSince1900 = __ntohl(netTime);

        // Unix time starts on Jan 1 1970.  Subtract seventy years:
        unixEpochTime = secsSince1900 - SEVENTY_YEARS;

        // now to convert Unix time (did you remember to set the
        // time zone first?)
        struct tm* local = localtime(&unixEpochTime);

        // convert from POSIX time struct to a Pico time struct
        datetime_t pico;
        pico.year = local->tm_year + 1900;
        pico.month = local->tm_mon + 1;
        pico.day = local->tm_mday;
        pico.dotw = local->tm_wday;
        pico.hour = local->tm_hour;
        pico.min = local->tm_min;
        pico.sec = local->tm_sec;

        // set the Pico RTC
        timeValid = rtc_set_datetime(&pico);
    }

    return (timeValid);
}

/********************************************************
 * timeString()
 ********************************************************
 * return a human-readable time string
 *******************************************************/
const std::string walltime::timeString()
{
    if (timeValid)
    {
        char ret[40];
        datetime_t now;

        // get the time from the RTC module
        bool ok = rtc_get_datetime(&now);
        
        if (!ok)
        {
            snprintf(ret, 39, "Error on rct_get_datetime()");
        }
        else
        {
            snprintf(ret, 39, "%02d:%02d:%02d",
                    now.hour, now.min, now.sec);
        }

        return (std::string(ret));
    }

    return (std::string("<not set>"));
}

/********************************************************
 * dateString()
 ********************************************************
 * return a human-readable date string
 *******************************************************/
const std::string walltime::dateString()
{
    if (timeValid)
    {
        char ret[40];
        datetime_t now;
        bool ok = rtc_get_datetime(&now);
        
        if (!ok)
        {
            snprintf(ret, 39, "Error on rct_get_datetime()");
        }
        else
        {        
            snprintf(ret, 39, "%d/%d/%d",
                    now.month, now.day, now.year);
        }

        return (std::string(ret));
    }

    return (std::string("<not set>"));
}

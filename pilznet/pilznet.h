

#ifndef PILZ_NET_H_
#define PILZ_NET_H

#include "../sys/walltime.h"
#include <string>
#include <cinttypes>

#include "pico/stdlib.h"

class pilznet
{
public:
    pilznet() {}
    ~pilznet() {}
    void init(void);

    bool connect(const std::string& ap, const std::string& pw);
    bool isConnected(void)                  { return (connected); }

    bool update(void);

    const std::string scan(void);

    const std::string getMac(void)          { return (macAddr); }
    const std::string getIP(void)           { return (ipAddr); }

    bool doNTP(const std::string& tz);
    bool isClockValid(void)                 { return (clockValid); }
    const std::string getTimeString(void);
    const std::string getDateString(void);
    const walltime getWallTime(void)        { return (wt); }

private:
    bool connected;
    bool clockValid;
    std::string ipAddr;
    std::string macAddr;
    walltime wt;

    const char* encryption2text(int thisType);
    const char* mac2text(uint8_t* mac);
    const char* status2text(int status); 
};

#endif // PILZ_NET_H_
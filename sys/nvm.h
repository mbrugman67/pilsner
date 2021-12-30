#ifndef NVM_H_
#define NVM_H_

#include <string>
#include <cstring>
#include "../ipc/mlogger.h"

class nvm
{
public:
    nvm() {}
    ~nvm() {}

    void write();
    void load();
    void init();

    void setTZ(const std::string& t);
    void setSSID(const std::string& s);         
    void setPwd(const std::string& p);

    const std::string getTZ()                   { return (nvmData.tz); }
    const std::string getSSID() const           { return (std::string(nvmData.ssid)); }
    const std::string getPwd() const            { return (std::string(nvmData.pw)); }
private:
    struct nvm_t
    {
        uint32_t signature;     // 0
        uint16_t setpoint;      // 4
        
        char ssid[64];          // 6
        char pw[64];            // 70
        char tz[32];            // 134

    } nvmData;                  // 166

    logger* log;
};

#endif // NVM_H_
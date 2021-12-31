/********************************************************
 * nvm.h
 ********************************************************
 * Handle storage of non-volatile data.  it is stored
 * in Flash memory, so don't overdo the writes!!
 * December 2021, M.Brugman
 * 
 *******************************************************/
#ifndef NVM_H_
#define NVM_H_

#include <string>
#include <cstring>
#include "pico/multicore.h"
#include "../ipc/mlogger.h"

class nvm
{
public:
    static nvm* getInstance();
    ~nvm() {}

    void write();
    void load();
    void init();
    void setDefaults();

    void setTZ(const std::string& t);
    void setSSID(const std::string& s);         
    void setPwd(const std::string& p);
    void setSetpoint(uint16_t sp)               { nvmData.setpoint = sp; }

    void dump2String();

    const std::string getTZ()                   { return (nvmData.tz); }
    const std::string getSSID() const           { return (std::string(nvmData.ssid)); }
    const std::string getPwd() const            { return (std::string(nvmData.pw)); }
    uint16_t getSetpoint() const                { return (nvmData.setpoint); }
private:
    struct nvm_t
    {
        uint32_t signature;     // 0
        uint16_t setpoint;      // 4
        
        char ssid[64];          // 6
        char pw[64];            // 70
        char tz[32];            // 134

    } nvmData;                  // 166
    
    static nvm* instance;
    nvm() {}

    logger* log;
    mutex_t mtx;
};

#endif // NVM_H_
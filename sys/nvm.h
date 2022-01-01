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

    void setCore1Ready(bool ready)              { core1Ready = ready; }

    void setTZ(const std::string& t);
    void setSSID(const std::string& s);         
    void setPwd(const std::string& p);
    void setSetpoint(float sp)                  { nvmData.setpoint = sp; }
    void setHysteresis (float h)                { nvmData.hysteresis = h; }

    void accumulateRuntime(uint32_t rt)         { nvmData.runtime += rt; }

    void dump2String();

    uint32_t getTotalRuntime()                  { return (nvmData.runtime); }
    const std::string getTZ()                   { return (nvmData.tz); }
    const std::string getSSID() const           { return (std::string(nvmData.ssid)); }
    const std::string getPwd() const            { return (std::string(nvmData.pw)); }
    float getSetpoint() const                   { return (nvmData.setpoint); }
    float getHysteresis() const                 { return (nvmData.hysteresis); }
private:
    bool core1Ready;

    struct nvm_t
    {
        uint32_t signature;     // 0
        uint32_t runtime;       // 4
        float setpoint;         // 8
        float hysteresis;       // 12
        
        char ssid[64];          // 16
        char pw[64];            // 80
        char tz[32];            // 144

        uint32_t endSig;        // 176
    } nvmData;                  // 180 total bytes
    
    static nvm* instance;
    nvm() {}

    logger* log;
    critical_section_t crit;
};

#endif // NVM_H_
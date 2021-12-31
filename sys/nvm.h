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
    void setSetpoint(float sp)                  { nvmData.setpoint = sp; }
    void setHysteresis (float h)                { nvmData.hysteresis = h; }

    void dump2String();

    const std::string getTZ()                   { return (nvmData.tz); }
    const std::string getSSID() const           { return (std::string(nvmData.ssid)); }
    const std::string getPwd() const            { return (std::string(nvmData.pw)); }
    float getSetpoint() const                   { return (nvmData.setpoint); }
    float getHysteresis() const                 { return (nvmData.hysteresis); }
private:
    struct nvm_t
    {
        uint32_t signature;     // 0
        float setpoint;         // 4
        float hysteresis;       // 8
        
        char ssid[64];          // 12
        char pw[64];            // 76
        char tz[32];            // 140

        uint32_t endSig;        // 172
    } nvmData;                  // 176 total bytes
    
    static nvm* instance;
    nvm() {}

    logger* log;
    mutex_t mtx;
};

#endif // NVM_H_
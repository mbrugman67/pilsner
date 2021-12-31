/********************************************************
 * nvm.cpp
 ********************************************************
 * Handle storage of non-volatile data.  it is stored
 * in Flash memory, so don't overdo the writes!!
 * December 2021, M.Brugman
 * 
 *******************************************************/
#include "hardware/flash.h"
#include "pico/stdlib.h"

#include "nvm.h"
#include "../creds.h"
#include "../ipc/mlogger.h"
#include "../utils/stringFormat.h"

#define sig (uint32_t)(0xabad1dea)              // signature to know we're valid
#define END_OF_FLASH    (uint32_t)(0x001f0000)  // 2 meg of flash on board
#define BLOCK_SIZE      (uint32_t)(0x00000100)  // block size is 256 bytes

nvm* nvm::instance = NULL;

/********************************************************
 * getInstance()
 ********************************************************
 * get the single instance of the class, instantiate
 * if necessary.
 * 
 * Note - there is no instance count or deletion - this
 * is embedded, baby; things stay for the duration!
 *******************************************************/
nvm* nvm::getInstance()
{    
    // If this is the first call, instantiate it and
    // do the init method
    if (!instance)
    {
        instance = new nvm;
        instance->init();
    }

    return (instance);
}

/********************************************************
 * write()
 ********************************************************
 * write to Flash memory
 *******************************************************/
void nvm::write()
{
    uint32_t startTime = to_ms_since_boot(get_absolute_time());
    
    // it is the last block in the flash chip
    uint32_t loc = END_OF_FLASH - BLOCK_SIZE;
    uint8_t data[BLOCK_SIZE];

    // so we have to write a full block
    std::memset(data, 0, BLOCK_SIZE);
    // but the actual data is less than that
    std::memcpy(data, (void*)&nvmData, sizeof(nvmData));

    mutex_enter_blocking(&mtx);
    flash_range_program(loc, data, 1);
    mutex_exit(&mtx);
}

/********************************************************
 * load()
 ********************************************************
 * read from Flash memory
 *******************************************************/
void nvm::load()
{
    // put our NVM data at the very end of flash
    const uint8_t* loc = (const uint8_t*)(XIP_BASE + END_OF_FLASH - BLOCK_SIZE);

    // get it
    mutex_enter_blocking(&mtx);
    std::memcpy((void*)&nvmData, loc, sizeof(nvmData));
    mutex_exit(&mtx);

    // is it good?
    if (nvmData.signature != sig)
    {
        this->setDefaults();
        this->write();
    }
}

/********************************************************
 * setDefaults()
 ********************************************************
 * will force a write
 *******************************************************/
void nvm::setDefaults()
{
    log->errWrite(stringFormat("Bad signature 0x%08x!  Resetting to defaults\n", nvmData.signature));

    nvmData.signature = sig;
    nvmData.setpoint = 65;
    strncpy(nvmData.ssid, WIFI_ACCESS_POINT_NAME, 63);
    strncpy(nvmData.pw, WIFI_PASSPHRASE, 63);
    strncpy(nvmData.tz, "CST6CDT", 31);

    this->write();
}

/********************************************************
 * dump2String()
 ********************************************************
 * write out the data -- NOTE: THIS WILL OUTPUT THE
 * WIFI PASSPHRASE IN CLEARTEXT.
 * 
 * You have been warned.
 *******************************************************/
void nvm::dump2String()
{
    log->dbgWrite("NVM:\n");
    log->dbgWrite(stringFormat("\n  Signature - 0x%08x\n"
                                 "   Setpoint - %d\n"
                                 "       SSID - %s\n"
                                 "         PW - %s\n"
                                 "   Timezone - %s\n",
            nvmData.signature,
            nvmData.setpoint,
            nvmData.ssid,
            nvmData.pw,
            nvmData.tz));
}

/********************************************************
 * init()
 ********************************************************
 * call after instantiation to do the first load and set
 * up the mutex
 *******************************************************/
void nvm::init()
{
    log = logger::getInstance();
    mutex_init(&mtx);
    this->load();
}

/********************************************************
 * setTZ()
 ********************************************************
 * relatively safe copy to Non vol struct.  You must call
 * write() if you want
 *******************************************************/
void nvm::setTZ(const std::string& s)
{
    size_t len = s.length();
    size_t max = sizeof(nvmData.tz);
    if (len >= max)
    {
        len = max - 1;
    }

    std::memcpy(nvmData.tz, s.c_str(), max);

    nvmData.tz[max - 1] = '\0';
}

/********************************************************
 * setSSID()
 ********************************************************
 * relatively safe copy to Non vol struct.  You must call
 * write() if you want
 *******************************************************/
void nvm::setSSID(const std::string& s)
{
    size_t len = s.length();
    size_t max = sizeof(nvmData.ssid);
    if (len >= max)
    {
        len = max - 1;
    }

    std::memcpy(nvmData.ssid, s.c_str(), max);

    nvmData.ssid[max - 1] = '\0';
}

/********************************************************
 * setPwd()
 ********************************************************
 * relatively safe copy to Non vol struct.  You must call
 * write() if you want
 *******************************************************/
void nvm::setPwd(const std::string& s)
{
    size_t len = s.length();
    size_t max = sizeof(nvmData.pw);
    if (len >= max)
    {
        len = max - 1;
    }

    std::memcpy(nvmData.pw, s.c_str(), max);

    nvmData.pw[max - 1] = '\0';
}
/********************************************************
 * nvm.cpp
 ********************************************************
 * Handle storage of non-volatile data.  it is stored
 * in Flash memory, so don't overdo the writes!!
 * December 2021, M.Brugman
 * 
 *******************************************************/
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#include "nvm.h"
#include "../creds.h"
#include "../ipc/mlogger.h"
#include "../utils/stringFormat.h"

#define SIG (uint32_t)(0xabad1dea)              // signature to know we're valid
#define ENDSIG (uint32_t)(0x2bad1dea)           // ending signature
#define END_OF_FLASH    (uint32_t)(0x001f0000)  // 2 meg of flash on board

// Offset is one sector from the end of the flash
#define START_OFFSET    (uint32_t)(END_OF_FLASH - FLASH_SECTOR_SIZE)

// Read location is offset from the start of the Execute-in-Place in memory map
#define READ_LOCATION   (uint32_t)(XIP_BASE + START_OFFSET)

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
    if (get_core_num() == 1)
    {
        log->errWrite("Cannot call nvm::write() from core 1!!\n");
        return;
    }

    // it is the last block in the flash chip
    uint32_t loc = START_OFFSET;

    // we write a full page at a crack
    uint8_t data[FLASH_PAGE_SIZE];

    // pointer to the start of the structure
    uint8_t* ptr = (uint8_t*)&nvmData;

    // total number of bytes to write
    size_t bytesRemaining = sizeof(nvmData);

    mutex_enter_blocking(&mtx);

    while (bytesRemaining)
    {
        // see how many bytes are remaining to be written.  if it's 
        // more than a block size, then write the next block
        size_t bytesToWrite = bytesRemaining;

        if (bytesToWrite >= FLASH_PAGE_SIZE)
        {
            bytesToWrite = FLASH_PAGE_SIZE;
        }

        // clear the buffer and copy the data to it
        std::memset(data, 0, FLASH_PAGE_SIZE);
        std::memcpy(data, ptr, bytesToWrite);

        // write the page to flash
        flash_range_program(loc, data, FLASH_PAGE_SIZE);

        // increment the starting address in the NVM structure
        ptr += FLASH_BLOCK_SIZE;

        // decrement by the number of bytes 
        bytesRemaining -= bytesToWrite;
    }
    mutex_exit(&mtx);
    log->dbgWrite("Done writing to Flash\n");
}

/********************************************************
 * load()
 ********************************************************
 * read from Flash memory
 *******************************************************/
void nvm::load()
{
    // put our NVM data at last sector of flash.  Reading from the flash
    // uses the address of the flash in the memory map (hence the XIP_BASE offset)
    const uint8_t* loc = (const uint8_t*)(READ_LOCATION);

    // get it
    //mutex_enter_blocking(&mtx);
    std::memcpy((void*)&nvmData, loc, sizeof(nvmData));
    //mutex_exit(&mtx);

    // is it good?
    if (nvmData.signature != SIG || nvmData.endSig != ENDSIG)
    {
        log->warnWrite("Setting NVM defaults\n");
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
    nvmData.signature = SIG;
    nvmData.setpoint = 65.0;
    nvmData.hysteresis = 1.0;
    strncpy(nvmData.ssid, WIFI_ACCESS_POINT_NAME, 63);
    strncpy(nvmData.pw, WIFI_PASSPHRASE, 63);
    strncpy(nvmData.tz, "CST6CDT", 31);
    nvmData.endSig = ENDSIG;
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
    log->dbgWrite(stringFormat("\n  Signature - 0x%08x\n"
                                 "   Setpoint - %02.1f\n"
                                 " Hysteresis - %02.1f\n"
                                 "       SSID - %s\n"
                                 "         PW - %s\n"
                                 "   Timezone - %s\n"
                                 "    End Sig - 0x%08x\n",
            nvmData.signature,
            nvmData.setpoint,
            nvmData.hysteresis,
            nvmData.ssid,
            nvmData.pw,
            nvmData.tz,
            nvmData.endSig));
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
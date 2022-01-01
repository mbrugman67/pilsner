/********************************************************
 * nvm.cpp
 ********************************************************
 * Handle storage of non-volatile data.  it is stored
 * in Flash memory, so don't overdo the writes!!
 ******************************************************** 
 *
 * The proper way to do this would be to add a serial 
 * EEPROM chip or something to the project and use that
 * to store config and non-vol stuff.  I don't have one
 * in my junk pile right now...
 * 
 * So, we'll use the Flash chip on the board for nvm.  
 * There's plenty of room - the chip is 2MB and the 
 * project is only about 140K.
 * 
 * The problem?  Both cores are executing code directly
 * from that chip (XIP - execute in place).  There's a 
 * resource contention for flash.  The easiest and most
 * ham-handed way to deal with it is to only allow one
 * core to do writes to flash.  The other core will be 
 * suspended during the write.  The SDK has API calls
 * for that!
 * 
 * Start with executing this on core 1:
 *          multicore_lockout_victim_init()
 * That will build in some extra code for core 1 which
 * is basically a busy loop/spinlock that executes
 * from RAM when core 0 requests it.
 * 
 * Before executing the Flash write, call this on core 0:
 *          multicore_lockout_start_blocking()
 * This begins the process of core 1 going into it's busy
 * loop and will block until core 1 is ready
 * 
 * Then do the stuff
 * 
 * Finally, on core 0, call:
 *          multicore_lockout_end_blocking()
 * and it will block until core 1 starts running again.
 * 
 * I admit I messed around with my own spinlock and __wfe()
 * and __sev() before realizing they had to run from RAM and
 * not flash.  Then I found these functions in the API.  Ugh.
 * 
 * This violates my "all blocking functions on core 1" 
 * requirement.  Testing shows that the erase/write cycle
 * for one flash block (256 bytes) takes between 25 and 29
 * milliseconds.  Given that it won't be done very often
 * I can live with that.
 * 
 ******************************************************** 
 * December 2021, M.Brugman
 * 
 *******************************************************/
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#include "nvm.h"
#include "../project.h"
#include "../creds.h"
#include "../ipc/mlogger.h"
#include "../ipc/ipc.h"
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

    // get the time now
    absolute_time_t start = get_absolute_time();

    // do the work in a critical section so we don't 
    // get clobbered by an interrupt or other surprises
    critical_section_enter_blocking(&crit);

#ifdef DEBUG
    log->dbgWrite("NVM::entered critical section\n");
#endif

    // ask core 1 to pause; it will go into a spinlock 
    // running from RAM.  Make sure to check if core 1 
    // has been init'd first, otherwise we could hang
    // here forever
    if (core1Ready)
    {
        multicore_lockout_start_timeout_us((uint64_t)25);
    }

#ifdef DEBUG
    log->dbgWrite("NVM::core 0 locked\n");
#endif

    // erase the full sector before writing
    flash_range_erase(START_OFFSET, FLASH_SECTOR_SIZE);

#ifdef DEBUG
    log->dbgWrite("NVM::erased sector\n");
#endif

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

    // tell core 1 it can start running from Flash again
    // Make sure to check if core 1 
    // has been init'd first, otherwise we could hang
    // here forever
    if (core1Ready)
    {
        multicore_lockout_end_timeout_us((uint64_t)25);
    }

#ifdef DEBUG
    log->dbgWrite("NVM::unlocked core 1\n");
#endif

    critical_section_exit(&crit);

#ifdef DEBUG
    log->dbgWrite("NVM::left critical section\n");
#endif

    log->dbgWrite(stringFormat("Done writing bytes to Flash in %ld us\n", absolute_time_diff_us(start, get_absolute_time())));
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

    // just a memcpy to get from Flash to RAM
    std::memcpy((void*)&nvmData, loc, sizeof(nvmData));

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
    nvmData.runtime = 0;
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
                                 "    Runtime - %d seconds\n"
                                 "   Setpoint - %02.1f deg F\n"
                                 " Hysteresis - %02.1f deg F\n"
                                 "       SSID - %s\n"
                                 "         PW - %s\n"
                                 "   Timezone - %s\n"
                                 "    End Sig - 0x%08x\n",
            nvmData.signature,
            nvmData.runtime,
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
    critical_section_init(&crit);
    core1Ready = false;
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
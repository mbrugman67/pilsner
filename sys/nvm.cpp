#include "hardware/flash.h"
#include "pico/stdlib.h"

#include "nvm.h"
#include "../ipc/mlogger.h"
#include "../utils/stringFormat.h"

#define sig (uint32_t)(0xabad1dea)
#define END_OF_FLASH    (uint32_t)(0x001fffff)  // 2 meg of flash on board
#define BLOCK_SIZE      (uint32_t)(0x00000100)  // block size is 256 bytes

void nvm::write()
{
    //uint32_t savedInterrupts = save_and_disable_interrupts();

    //restore_interrupts(savedInterrupts);
}

void nvm::load()
{
    const uint8_t* loc = (const uint8_t*)(XIP_BASE + END_OF_FLASH - BLOCK_SIZE);

    std::memcpy((void*)&nvmData, loc, sizeof(nvmData));

    log->dbgWrite(stringFormat("Signature byte: 0x%08x\n", nvmData.signature));
}

void nvm::init()
{
    log = logger::getInstance();
    this->load();
}


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
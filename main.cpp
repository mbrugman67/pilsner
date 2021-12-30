/********************************************************
 * main.cpp
 ********************************************************
 * Entry point for pilsner project.  Initialize stuffs, 
 * start it up.  Start up core 1 and let it do stuff
 * 8 April 2021, M.Brugman
 * 
 *******************************************************/

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include <stdio.h>
#include <cstring>
#include <string>
#include <ctime>
#include <vector>

#include "project.h"
#include "core1.h"
#include "./sys/ir.h"
#include "./ipc/ipc.h"
#include "./ipc/mlogger.h"
#include "./sys/walltime.h"
#include "./utils/stringFormat.h"
#include "./sys/nvm.h"

static inter_core_t ipcCore0Data;
static uint32_t msTick = 0;

void heartBeatLED()
{
    static bool first = true;
    static uint32_t startTime = 0;

    if (first)
    {
        first = false;
        startTime = to_ms_since_boot(get_absolute_time());
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    }

    uint32_t elapsedTime = to_ms_since_boot(get_absolute_time()) - startTime;

    if (elapsedTime < 666)          gpio_put(PICO_DEFAULT_LED_PIN, 1);
    else if (elapsedTime < 1000)    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    else                            startTime = to_ms_since_boot(get_absolute_time());
}

bool ledIRTest()
{
    static bool inProgress = false;
    static ir infra(PIN_IR);
    static uint8_t value = 0;
    static uint32_t startTime = 0;
    static logger* log = logger::getInstance();

    if (infra.isCodeAvailable())
    {
        irData_t raw = infra.peekRawCode();
        irKey_t cmd = infra.getKeyCode();
        if (cmd != KEY_NULL)
        {
            switch (cmd)
            {
                case KEY_0:
                {
                    log->dbgWrite(stringFormat("%s::seconds since boot: %d\n", __FUNCTION__, to_ms_since_boot(get_absolute_time() / 1000)));
                }  break;

                case KEY_1:
                {
                    log->dbgWrite(stringFormat("%s::%s %s\n", __FUNCTION__,
                        walltime::timeString().c_str(), walltime::dateString().c_str()));
                }  break;

                case KEY_2:
                {
                    if (!inProgress)
                    {
                        log->dbgWrite(stringFormat("%s::Asking for IP Address\n", __FUNCTION__));
                        startTime = msTick;
                        inProgress = true;
                        ipcCore0Data.cmd = IS_GET_IP;
                        updateSharedData(US_NEW_CMD, ipcCore0Data);
                    }
                }  break;

                case KEY_3:
                {
                    if (!inProgress)
                    {
                        log->dbgWrite(stringFormat("%s::Asking for MAC Address\n", __FUNCTION__));
                        startTime = msTick;
                        inProgress = true;
                        ipcCore0Data.cmd = IS_GET_MAC;
                        updateSharedData(US_NEW_CMD, ipcCore0Data);
                    }
                }  break;

                case KEY_4:
                {
                    if (!inProgress)
                    {
                        log->dbgWrite(stringFormat("%s::Asking for net scan\n", __FUNCTION__));
                        startTime = msTick;
                        inProgress = true;
                        ipcCore0Data.cmd = IS_DO_SCAN;
                        updateSharedData(US_NEW_CMD, ipcCore0Data);
                    }
                }  break;

                case KEY_5:
                {
                }  break;

                case KEY_6:
                {
                }  break;

                case KEY_7:
                {
                }

                case KEY_OK:
                {
                    dumpStruct(ipcCore0Data, "Core 0");
                }
            }
        }
        else
        {
            log->warnWrite(stringFormat("%s::Unknown: 0x%02xn", __FUNCTION__, cmd));
        }
    }

    if (inProgress && ipcCore0Data.cmd == IS_NO_CMD)
    {
        inProgress = false;
        uint32_t elapsed = msTick - startTime;

        if (ipcCore0Data.ack == IS_GET_IP)
        {
            log->dbgWrite(stringFormat("%s::Got IP Addr %s in %dms\n", __FUNCTION__, ipcCore0Data.ipAddress.c_str(), elapsed));
        }
        else if (ipcCore0Data.ack == IS_GET_MAC)
        {
            log->dbgWrite(stringFormat("%s::Got MAC Addr %s in %dms\n", __FUNCTION__, ipcCore0Data.macAddress.c_str(), elapsed));
        }
        else if (ipcCore0Data.ack == IS_DO_SCAN)
        {
            log->dbgWrite(stringFormat("%s::Scan done in %dms:\n", __FUNCTION__, elapsed));

            log->dbgWrite(stringFormat(" Found %d access points:\n", ipcCore0Data.scanResult.count));
            std::vector<ap_data_t>::const_iterator cit = ipcCore0Data.scanResult.apData.begin();
            while (cit != ipcCore0Data.scanResult.apData.end())
            {
                log->dbgWrite(stringFormat("  Name: %s\n", cit->ssid.c_str()));
                log->dbgWrite(stringFormat("    BSSID: %s\n", cit->bssid.c_str()));
                log->dbgWrite(stringFormat("    Strength: %ddbm\n", cit->strength));
                log->dbgWrite(stringFormat("    Channel: %d\n", cit->channel));
                log->dbgWrite(stringFormat("    Encryption: %s\n", cit->encryption.c_str()));
                ++cit;
            }
        }

        ipcCore0Data.ack = IS_NO_CMD;
        updateSharedData(US_ACK_CMD, ipcCore0Data);
    }

    return (true);
}

/****************************************************
 * Main
 ****************************************************/
int main()
{
    stdio_init_all();
    logger* log = logger::getInstance();
    log->initBuffer();

    log->dbgWrite("\n****************starting****************\n");

    if (watchdog_enable_caused_reboot())
    {
        log->dbgWrite("Rebooted by watchdog!\n");
    }

    if (initIPC())
    {
        multicore_launch_core1(core1Main);
    }

    nvm data;
    data.init();

    msTick = to_ms_since_boot(get_absolute_time());
    uint32_t lastMs = msTick;
    uint8_t tick = 0;

    updateSharedData(US_NONE, ipcCore0Data);

    while (true)
    { 
        switch (tick)
        {
            case 0:
            {
                heartBeatLED();
            }  break;
            
            case 1:
            {
                ledIRTest();
            }  break;
            
            case 2:
            {
                static uint32_t lastTempCount = 0;
                static float lastTemp = 0.0;

                if (lastTempCount != ipcCore0Data.tempCount && lastTemp != ipcCore0Data.temperatue)
                {
                    log->infoWrite(stringFormat("%02.1f\n", ipcCore0Data.temperatue));
                }
                
                lastTempCount = ipcCore0Data.tempCount;
                lastTemp = ipcCore0Data.temperatue;
            }  break;
            
            case 3:
            {
                updateSharedData(US_NONE, ipcCore0Data);
            }  break;
        }

        // tight part of service loop
        while(msTick == lastMs)
        {
            msTick = to_ms_since_boot(get_absolute_time());
        }

        lastMs = msTick;
        
        ++tick;
        tick %= 4;
    }
}

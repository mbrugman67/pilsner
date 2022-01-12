/********************************************************
 * main.cpp
 ********************************************************
 * Entry point for pilsner project.  Initialize stuffs, 
 * start it up.  Start up core 1 and let it do stuff
 * December 2021, M.Brugman
 * 
 ********************************************************
 * To find the size of the binary that will be loaded
 * into flash:
 * 
 * $ objdump --all-headers pilsner.elf | grep -i  flash_binary_end
 *   100207e8 g       .ARM.attributes	00000000 __flash_binary_end
 * 
 * Flash start is 0x1000000, so subtract that from 0x100207e8
 * and the application size is 0x000207e8, or 133069 bytes.
 * The flash chip is 2Meg
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
#include "reefer.h"
#include "hub75.h"
#include "./sys/ir.h"
#include "./ipc/ipc.h"
#include "./ipc/mlogger.h"
#include "./sys/walltime.h"
#include "./utils/stringFormat.h"
#include "./sys/nvm.h"

static inter_core_t ipcCore0Data;   // for sharing data between cores
static uint32_t msTick = 0;         // tick counter
static nvm* data = NULL;            // non-vol data storage handler

/********************************************
 * heartBeatLED()
 ********************************************
 * Generic blinky light
********************************************/
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

    // blink the LED with a 1Hz frequency 66% duty cycle
    if (elapsedTime < 666)          gpio_put(PICO_DEFAULT_LED_PIN, 1);
    else if (elapsedTime < 1000)    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    else                            startTime = to_ms_since_boot(get_absolute_time());
}

/********************************************
 * ledIRTest()
 ********************************************
 * Placeholder for I/R remote interface
********************************************/
bool ledIRTest()
{
    static bool inProgress = false;
    static ir infra(PIN_IR);
    static uint8_t value = 0;
    static uint32_t startTime = 0;
    static logger* log = logger::getInstance();

    // was there a code received?
    if (infra.isCodeAvailable())
    {
        irData_t raw = infra.peekRawCode();     // save the raw code in case it's an "unknown"
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
                    data->dump2String();
                }  break;

                case KEY_6:
                {
                    log->dbgWrite("Read NVM\n");
                    data->load();
                    data->dump2String();
                }  break;

                case KEY_7:
                {
                    log->dbgWrite("Write NVM\n");
                    data->write();
                }  break;

                case KEY_8:
                {
                    log->dbgWrite("Set NVM defaults\n");
                    data->setDefaults();
                    data->write();
                }  break;

                case KEY_9:
                {
                    log->dbgWrite(stringFormat("Current temp %0.2f\n", ipcCore0Data.temperatue));
                }  break;

                case KEY_UP:
                {
                    data->setSetpoint(99.0);
                    log->dbgWrite("setpoint to 99.0\n");
                }  break;

                case KEY_DOWN:
                {
                    data->setSetpoint(32.0);
                    log->dbgWrite("setpoint to 32.0\n");
                }  break;

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

    // test code - handle response from core 1 for commands issued by I/R
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
 * main()
 ****************************************************/
int main()
{
    // init the standard library
    stdio_init_all();

// if debugging, add time to get the serial terminal console 
// on the host running
#ifdef DEBUG
    sleep_ms(6000);
#endif

    // get the single instane of the global logger
    logger* log = logger::getInstance();

    // log separator
    log->dbgWrite("*********\n");
    log->dbgWrite("* start *\n");
    log->dbgWrite("*********\n");

    // get the global singleton persisted data handler.  
    data = nvm::getInstance();
    data->init();
    sleep_ms(50);

    // no real reason for this.  Reboot can be commanded by a 
    // UDP network connection, just drop a line in the log if 
    // that happened.  Maybe we'll do different things based on
    // reset type
    if (watchdog_enable_caused_reboot())
    {
        log->dbgWrite("Rebooted by watchdog!\n");
    }
    else
    {
        log->dbgWrite("Cold boot\n");
    }

    // start up core 1.  Core 0 (this core) is going to handle 
    // more time-critical stuffs; anything that may block will
    // be on core 1.  The interprocess comms handler (IPC) is
    // a custom thing instead of using the FIFO provided by the 
    // SDK.
    if (initIPC())
    {
        multicore_launch_core1(core1Main);
        data->setCore1Ready(true);
    }

    // instantiate the refregeration pump object
    reefer chill;
    chill.init();
    bool pumpRunning = false;

    // instantiate the display panel
    hub75 display;
    display.init();

    // start getting the timing stuffs
    msTick = to_ms_since_boot(get_absolute_time());
    uint32_t lastMs = msTick;
    uint8_t tick = 0;

    // Do the first inter-core update
    updateSharedData(US_NONE, ipcCore0Data);

    // main loop - each task gets hit once every 4 ms
    while (true)
    { 
        switch (tick)
        {
            // Task 1 - heartbeat and minor stuffs
            case 0:
            {
                heartBeatLED();
            }  break;
            
            // Task 2 - I/R and display UI
            case 1:
            {
                ledIRTest();
            }  break;
            
            // task 3 - reefer control
            // (reefer is slang for refrigeration, not weed in this case)
            case 2:
            {
                static reefer_state_t lastState = RS_IDLE;

                chill.update(ipcCore0Data.temperatue);
                pumpRunning = chill.isPumpRunning();

                // drop a line in the log to indicate state change
                if (lastState != chill.getReeferState())
                {
                    // accumulate runtime in NVM
                    if (lastState == RS_CHILLING)
                    {
                        data->accumulateRuntime(chill.getPumpRuntimeSeconds());
                        data->write();
                    }

                    log->dbgWrite(stringFormat("Reefer state from %s to %s\n",
                        chill.getStateName(lastState).c_str(),
                        chill.getStateName(chill.getReeferState()).c_str()));
                }

                lastState = chill.getReeferState();

            }  break;
            
            // Task 3 - inter-core comms updates
            case 3:
            {
                updateSharedData(US_NONE, ipcCore0Data);
            }  break;
        }

        display.update();

        // tight part of service loop
        while(msTick == lastMs)
        {
            // wait for next ms tick
            msTick = to_ms_since_boot(get_absolute_time());
        }

        lastMs = msTick;
        
        ++tick;
        tick %= 4;
    }
}

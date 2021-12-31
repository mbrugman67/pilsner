/********************************************************
 * core1.cpp
 ********************************************************
 * Entry point for second core
 * December 2021, M.Brugman
 * 
 *******************************************************/

#include <stdio.h>
#include <cstring>
#include <string>

#include "pico/stdlib.h"

#include "./ipc/ipc.h"
#include "./sys/nvm.h"
#include "./pilznet/pilznet.h"
#include "./ipc/mlogger.h"
#include "./sys/walltime.h"
#include "./utils/stringFormat.h"
#include "./ds1820/ds1820.h"
#include "creds.h"

static pilznet pnet;
static ds1820 probe;
static inter_core_t ipcCore1Data;

/*********************************************************
 * core1Main()
 *********************************************************
 * main loop for core 1
 ********************************************************/
void core1Main(void)
{
    // get the global singleton instances for the logger
    // and non-vol data storage handlers
    logger* log = logger::getInstance();
    nvm* data = nvm::getInstance();

    // Weeeee're here!
    log->dbgWrite(stringFormat("%s::Starting\n", __FUNCTION__));

    uint32_t ms = to_ms_since_boot(get_absolute_time());
    uint32_t lastMs = ms;
    uint8_t tick = 0;

    // Initialize the Wifi and connect to the specified 
    // access point - this could take up to 6 seconds
    pnet.init();
    pnet.connect(data->getSSID(), data->getPwd());

    // go get the current TOD and date from the interwebz
    pnet.doNTP(data->getTZ());


    ipcCore1Data.wifiConnected = pnet.isConnected();
    ipcCore1Data.clockReady = pnet.isClockValid();

    // log the IP address.  Also spit it out the USB serial interface
    // for fallback 
    log->dbgWrite(stringFormat("%s()::%s\n", __FUNCTION__, pnet.getIP().c_str()));
    printf("%s()::%s\n", __FUNCTION__, pnet.getIP().c_str());
    
    // init temperature sensor PIO state machine
    uint32_t sm = probe.init((PIO)pio0, 15);

    // do the first inter-core process update
    updateSharedData(US_CORE1_READY | US_WIFI_CONNECTED | US_CLOCK_READY, ipcCore1Data);

    // main loop.  Nominally, each task will hit once every 5ms, but some of them
    // will block, so who knows?
    while (true)
    { 
        uint16_t updates = US_NONE;
        
        switch (tick)
        {   
            // core 0 asked for our IP address, this will return right
            // away (assuming we are connected)
            case 0:
            {
                if (ipcCore1Data.cmd == IS_GET_IP)
                {
                    log->dbgWrite(stringFormat("%s::Get IP Address\n", __FUNCTION__));

                    updates = US_NEW_CMD;
                    ipcCore1Data.cmd = IS_NO_CMD;

                    if (pnet.isConnected())
                    {
                        ipcCore1Data.ack = IS_GET_IP;
                        ipcCore1Data.ipAddress = pnet.getIP();
                        updates |= US_IP_ADDR | US_ACK_CMD;
                    }
                    
                    updateSharedData(updates, ipcCore1Data);
                }
            }  break;
            
            // core 0 asked for our MAC address, this will return right
            // away (assuming we are connected)
            case 1:
            {
                if (ipcCore1Data.cmd == IS_GET_MAC)
                {
                    log->dbgWrite(stringFormat("%s::Get MAC address\n", __FUNCTION__));

                    updates = US_NEW_CMD;
                    ipcCore1Data.cmd = IS_NO_CMD;

                    if (pnet.isConnected())
                    {
                        ipcCore1Data.ack = IS_GET_MAC;
                        ipcCore1Data.macAddress = pnet.getMac();
                        updates |= US_MAC_ADDR | US_ACK_CMD;
                    }
                    
                    updateSharedData(updates, ipcCore1Data);
                }
            }  break;
            
            // Core 0 asked for a scan of all available networks.  This will
            // block for several seconds
            case 2:
            {
                if (ipcCore1Data.cmd == IS_DO_SCAN)
                {
                    log->dbgWrite(stringFormat("%s::Starting net scan\n", __FUNCTION__));

                    updates = US_NEW_CMD;
                    ipcCore1Data.cmd = IS_NO_CMD;

                    if (pnet.isConnected())
                    {
                        ipcCore1Data.ack = IS_DO_SCAN;
                        ipcCore1Data.scanResult = pnet.scan();
                        updates |= US_SCAN_DATA | US_ACK_CMD;
                    }
                    
                    updateSharedData(updates, ipcCore1Data);
                }
            }  break;
            
            // Read the temperature probe and push to core 0; probe read update
            // rate is actually quite a bit less often than once every 5ms.  This 
            // will block for about 25ms
            case 3:
            {
                static uint16_t count = 0;

                if (!(count % 500))
                {
                    ipcCore1Data.temperatue = probe.getTemperature();
                    ++ipcCore1Data.tempCount;

                    updateSharedData(US_NEW_TMP_DATA, ipcCore1Data);
                }
                ++count;
            }  break;

            // update the network handler - this is for the udp
            // server
            case 4:
            {
                pnet.update();
                updateSharedData(IS_NO_CMD, ipcCore1Data);
            }  break;
        }

        // tight part of service loop, wait for next ms tick
        while(ms == lastMs)
        {
            ms = to_ms_since_boot(get_absolute_time());
        }

        lastMs = ms;
        
        ++tick;
        tick %= 5;
    }
}
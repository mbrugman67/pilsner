#include <stdio.h>
#include <cstring>
#include <string>

#include "pico/stdlib.h"

#include "./ipc/ipc.h"
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
    logger* log = logger::getInstance();

    log->dbgWrite(stringFormat("%s::Starting\n", __FUNCTION__));

    uint32_t ms = to_ms_since_boot(get_absolute_time());
    uint32_t lastMs = ms;
    uint8_t tick = 0;

    // first thing is to initialize the Wifi
    pnet.init();
    pnet.connect(WIFI_ACCESS_POINT_NAME, WIFI_PASSPHRASE);
    pnet.doNTP("CST6CDT");

    ipcCore1Data.wifiConnected = pnet.isConnected();
    ipcCore1Data.clockReady = pnet.isClockValid();

    log->dbgWrite(stringFormat("%s()::%s\n", __FUNCTION__, pnet.getIP().c_str()));
    
    // init temperature sensor
    uint32_t sm = probe.init((PIO)pio0, 15);

    // do the first update
    updateSharedData(US_CORE1_READY | US_WIFI_CONNECTED | US_CLOCK_READY, ipcCore1Data);

    while (true)
    { 
        uint16_t updates = US_NONE;
        
        switch (tick)
        {   
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

            case 4:
            {
                pnet.update();
                updateSharedData(IS_NO_CMD, ipcCore1Data);
            }  break;
        }

        // tight part of service loop
        while(ms == lastMs)
        {
            ms = to_ms_since_boot(get_absolute_time());
        }

        lastMs = ms;
        
        ++tick;
        tick %= 5;
    }
}
#include <stdio.h>
#include <cstring>
#include <string>

#include "pico/stdlib.h"

#include "./ipc/ipc.h"
#include "./pilznet/pilznet.h"
#include "creds.h"

static pilznet pnet;
static inter_core_t ipcCore1Data;

/*********************************************************
 * core1Main()
 *********************************************************
 * main loop for core 1
 ********************************************************/
void core1Main(void)
{
    printf("%s::Entering\n", __FUNCTION__);

    uint32_t ms = to_ms_since_boot(get_absolute_time());
    uint32_t lastMs = ms;
    uint8_t tick = 0;

    // first thing is to initialize the Wifi
    pnet.init();
    ipcCore1Data.wifiConnected = pnet.connect(WIFI_ACCESS_POINT_NAME, WIFI_PASSPHRASE);
    ipcCore1Data.clockReady = pnet.doNTP("CST6CDT");

    printf("%s::Network and clock initialized\n", __FUNCTION__);

    updateSharedData(US_CORE1_READY | US_WIFI_CONNECTED | US_CLOCK_READY, ipcCore1Data);

    printf("%s::First shared data update done\n", __FUNCTION__);

    while (true)
    { 
        uint16_t updates = US_NONE;
        
        switch (tick)
        {
            case 0:
            {
                if (ipcCore1Data.cmd == IS_GET_CLOCK_TIME)
                {
                    printf("%s::Get clock time\n", __FUNCTION__);

                    updates = US_NEW_CMD;
                    ipcCore1Data.cmd = IS_NO_CMD;

                    if (pnet.isClockValid())
                    {
                        ipcCore1Data.ack = IS_GET_CLOCK_TIME;
                        ipcCore1Data.clockTime = pnet.getDateTime();
                        updates |= US_CLOCK_TIME | US_ACK_CMD;
                        ipcCore1Data.ack = IS_GET_CLOCK_TIME;
                    }
                    
                    updateSharedData(updates, ipcCore1Data);
                }
            }  break;
            
            case 2:
            {
                if (ipcCore1Data.cmd == IS_GET_IP)
                {
                    printf("%s::Get IP Address\n", __FUNCTION__);

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
            
            case 4:
            {
                if (ipcCore1Data.cmd == IS_GET_MAC)
                {
                    printf("%s::Get MAC address\n", __FUNCTION__);

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
            
            case 6:
            {
                if (ipcCore1Data.cmd == IS_DO_SCAN)
                {
                    printf("%s::Starting net scan\n", __FUNCTION__);

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
            
            case 8:
            {
                pnet.update();
                        
                //inter_core_t t;
                //copyIPC(ipcCore1Data, t);
                updateSharedData(IS_NO_CMD, ipcCore1Data);
                //diffStruct(t, ipcCore1Data, 1);
            }  break;
        }



        // tight part of service loop
        while(ms == lastMs)
        {
            ms = to_ms_since_boot(get_absolute_time());
        }

        lastMs = ms;
        
        ++tick;
        tick %= 10;
    }
}
/********************************************************
 * main.cpp
 ********************************************************
 * Entry point for pilsner project.  Initialize stuffs, 
 * start it up.  Start up core 1 and let it do stuff
 * 8 April 2021, M.Brugman
 * 
 *******************************************************/

#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

#include "project.h"
#include "./pilznet/pilznet.h"
#include "./sys/ir.h"
#include "creds.h"

static pilznet pnet;

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

void ledIRTest()
{
    static bool first = true;

    static ir infra(PIN_IR);
    
    static uint8_t value = 0;    

    if (infra.isCodeAvailable())
    {
        irData_t raw = infra.peekRawCode();
        printf("<<0x%02x>>", raw.b.cmd);

        irKey_t cmd = infra.getKeyCode();
        if (cmd != KEY_NULL)
        {
            printf("Key %s\n", infra.getKeyName(cmd).c_str());

            switch (cmd)
            {
                case KEY_0:
                {
                    printf("ms since boot: %d\n", to_ms_since_boot(get_absolute_time()));
                }  break;

                case KEY_1:
                {
                    pnet.scan();
                }  break;

                case KEY_2:
                {
                }  break;

                case KEY_3:
                {
                    printf(pnet.getTimeString().c_str());
                    printf("\n");
                }  break;

                case KEY_4:
                {
                    printf(pnet.getDateString().c_str());
                    printf("\n");
                }  break;

                case KEY_5:
                {
                    printf(pnet.getIP().c_str());
                    printf("\n");
                }  break;

                case KEY_6:
                {
                    printf(pnet.getMac().c_str());
                    printf("\n");
                }  break;

                case KEY_7:
                {
                }
            }
        }
        else
        {
            printf("Unknown\n");
        }
    }
}

/****************************************************
 * Main
 ****************************************************/
int main()
{
    stdio_init_all();
    pnet.init();

    sleep_ms(2000);
    printf("starting...\n");

    pnet.connect(WIFI_ACCESS_POINT_NAME, WIFI_PASSPHRASE);
    printf("Connected!\n");
    pnet.doNTP("CST6CDT");

    uint32_t ms = to_ms_since_boot(get_absolute_time());
    uint32_t lastMs = ms;
    uint8_t tick = 0;

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
                pnet.update();
            }  break;
            
            case 3:
            {

            }  break;
        }

        // tight part of service loop
        while(ms == lastMs)
        {
            ms = to_ms_since_boot(get_absolute_time());
        }

        lastMs = ms;
        
        ++tick;
        tick %= 4;
    }
}

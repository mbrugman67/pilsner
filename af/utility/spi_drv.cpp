/*
  spi_drv.cpp - Library for Arduino Wifi shield.
  Copyright (c) 2018 Arduino SA. All rights reserved.
  Copyright (c) 2011-2014 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/irq.h"

#include "spi_drv.h"

//#define _DEBUG_
extern "C"
{
#include "debug.h"
}

static bool inverted_reset = false;

// GPIO pins
int8_t pMiso = 0;
int8_t pClock = 1;
int8_t pCS = 2;
int8_t pMosi = 3;

int8_t pReady = 7;
int8_t pReset = 5; 
int8_t pGPIO0 = 6;

#define DELAY_TRANSFER()

bool SpiDrv::initialized = false;

void SpiDrv::begin()
{
    gpio_init(pReady);
    gpio_set_dir(pReady, GPIO_IN);
    gpio_set_pulls(pReady, true, false);
    
    if (pGPIO0 >= 0)
    {
        gpio_init(pGPIO0);
        gpio_set_dir(pGPIO0, GPIO_OUT);
        gpio_put(pGPIO0, 1);
    }

    gpio_init(pReset);
    gpio_set_dir(pReset, GPIO_OUT);

    gpio_put(pReset, inverted_reset ? 1 : 0);
    sleep_ms(10);
    gpio_put(pReset, inverted_reset ? 0 : 1);
    sleep_ms(750);

    gpio_init(pCS);
    gpio_set_dir(pCS, GPIO_OUT);
    gpio_put(pCS, 1);

    if (pGPIO0 >= 0)
    {
        gpio_set_dir(pGPIO0, GPIO_IN);
        gpio_set_pulls(pGPIO0, true, false);
    }

    spi_init(spi0, 8 * 1000 * 1000);
    gpio_set_function(pMiso, GPIO_FUNC_SPI);
    gpio_set_function(pClock, GPIO_FUNC_SPI);
    gpio_set_function(pMosi, GPIO_FUNC_SPI);
    //bi_decl(bi_1pin_with_name(pCS, "SPI CS"));

    //bi_decl(bi_3pins_with_func(g_misoPin, g_mosiPin, g_clockPin, GPIO_FUNC_SPI));
    spi_set_format (spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

#ifdef _DEBUG_
    INIT_TRIGGER()
#endif

    initialized = true;
}

void SpiDrv::end()
{
    gpio_put(pReset, inverted_reset ? 1 : 0);

    gpio_set_dir(pCS, GPIO_IN);

    initialized = false;
}

void SpiDrv::spiSlaveSelect()
{
    gpio_put(pCS, 0);

    unsigned long startTime = to_ms_since_boot(get_absolute_time());
    unsigned long nowTime = startTime;

    // wait for up to 5 ms for the NINA to indicate it is not ready for transfer
    // the timeout is only needed for the case when the shield or module is not present
    while (nowTime - startTime < 5)
    {
        nowTime = to_ms_since_boot(get_absolute_time());
        if (gpio_get(pReady) == 1)
        {
            break;
        }
    }
}

void SpiDrv::spiSlaveDeselect()
{
    gpio_put(pCS, 1);
}

char SpiDrv::spiTransfer(volatile char data)
{
    uint8_t rcv = 0;
    uint8_t snd = data;
    spi_write_read_blocking (spi0, &snd, &rcv, 1);
    DELAY_TRANSFER();

    return ((char)rcv); // return the received byte
}

int SpiDrv::waitSpiChar(uint8_t waitChar)
{
    int timeout = TIMEOUT_CHAR;
    uint8_t _readChar = 0;
    do
    {
        spi_read_blocking (spi0, 0x00, &_readChar, 1);
        if (_readChar == ERR_CMD)
        {
            WARN("Err cmd received\n");
            return (-1);
        }
    } while ((timeout-- > 0) && (_readChar != waitChar));
    
    return (_readChar == waitChar);
}

int SpiDrv::readAndCheckChar(char checkChar, char *readChar)
{
    getParam((uint8_t *)readChar);

    return (*readChar == checkChar);
}

char SpiDrv::readChar()
{
    uint8_t readChar = 0;
    getParam(&readChar);
    return readChar;
}

#define WAIT_START_CMD(x) waitSpiChar(START_CMD)

#define IF_CHECK_START_CMD(x)            \
    if (!WAIT_START_CMD(_data))          \
    {                                    \
        TOGGLE_TRIGGER()                 \
        WARN("Error waiting START_CMD"); \
        return 0;                        \
    }                                    \
    else

#define CHECK_DATA(check, x)          \
    if (!readAndCheckChar(check, &x)) \
    {                                 \
        TOGGLE_TRIGGER()              \
        WARN("Reply error");          \
        INFO2(check, (uint8_t)x);     \
        return 0;                     \
    }                                 \
    else

#define waitSlaveReady() (gpio_get(pReady) == 0)
#define waitSlaveSign() (gpio_get(pReady) == 1)
#define waitSlaveSignalH()                  \
    while (gpio_get(pReady) != 1)           \
    {                                       \
    }
#define waitSlaveSignalL()                  \
    while (gpio_get(pReady) != 0)           \
    {                                       \
    }

void SpiDrv::waitForSlaveSign()
{
    while (!waitSlaveSign())
        ;
}

void SpiDrv::waitForSlaveReady()
{
    while (!waitSlaveReady())
        ;
}

void SpiDrv::getParam(uint8_t *param)
{
    // Get Params data
    *param = spiTransfer(DUMMY_DATA);
    DELAY_TRANSFER();
}

int SpiDrv::waitResponseCmd(uint8_t cmd, uint8_t numParam, uint8_t *param, uint8_t *param_len)
{
    char _data = 0;
    int ii = 0;

    IF_CHECK_START_CMD(_data)
    {
        CHECK_DATA(cmd | REPLY_FLAG, _data){};

        CHECK_DATA(numParam, _data)
        {
            readParamLen8(param_len);
            for (ii = 0; ii < (*param_len); ++ii)
            {
                // Get Params data
                //param[ii] = spiTransfer(DUMMY_DATA);
                getParam(&param[ii]);
            }
        }

        readAndCheckChar(END_CMD, &_data);
    }

    return 1;
}
/*
int SpiDrv::waitResponse(uint8_t cmd, uint8_t numParam, uint8_t* param, uint16_t* param_len)
{
    char _data = 0;
    int i =0, ii = 0;

    IF_CHECK_START_CMD(_data)
    {
        CHECK_DATA(cmd | REPLY_FLAG, _data){};

        CHECK_DATA(numParam, _data);
        {
            readParamLen16(param_len);
            for (ii=0; ii<(*param_len); ++ii)
            {
                // Get Params data
                param[ii] = spiTransfer(DUMMY_DATA);
            } 
        }         

        readAndCheckChar(END_CMD, &_data);
    }     
    
    return 1;
}
*/

int SpiDrv::waitResponseData16(uint8_t cmd, uint8_t *param, uint16_t *param_len)
{
    char _data = 0;
    uint16_t ii = 0;

    IF_CHECK_START_CMD(_data)
    {
        CHECK_DATA(cmd | REPLY_FLAG, _data){};

        uint8_t numParam = readChar();
        if (numParam != 0)
        {
            readParamLen16(param_len);
            for (ii = 0; ii < (*param_len); ++ii)
            {
                // Get Params data
                param[ii] = spiTransfer(DUMMY_DATA);
            }
        }

        readAndCheckChar(END_CMD, &_data);
    }

    return 1;
}

int SpiDrv::waitResponseData8(uint8_t cmd, uint8_t *param, uint8_t *param_len)
{
    char _data = 0;
    int ii = 0;

    IF_CHECK_START_CMD(_data)
    {
        CHECK_DATA(cmd | REPLY_FLAG, _data){};

        uint8_t numParam = readChar();
        if (numParam != 0)
        {
            readParamLen8(param_len);
            for (ii = 0; ii < (*param_len); ++ii)
            {
                // Get Params data
                param[ii] = spiTransfer(DUMMY_DATA);
            }
        }

        readAndCheckChar(END_CMD, &_data);
    }

    return 1;
}

int SpiDrv::waitResponseParams(uint8_t cmd, uint8_t numParam, tParam *params)
{
    char _data = 0;
    int i = 0, ii = 0;

    IF_CHECK_START_CMD(_data)
    {
        CHECK_DATA(cmd | REPLY_FLAG, _data){};

        uint8_t _numParam = readChar();
        if (_numParam != 0)
        {
            for (i = 0; i < _numParam; ++i)
            {
                params[i].paramLen = readParamLen8();
                for (ii = 0; ii < params[i].paramLen; ++ii)
                {
                    // Get Params data
                    params[i].param[ii] = spiTransfer(DUMMY_DATA);
                }
            }
        }
        else
        {
            WARN("Error numParam == 0");
            return 0;
        }

        if (numParam != _numParam)
        {
            WARN("Mismatch numParam");
            return 0;
        }

        readAndCheckChar(END_CMD, &_data);
    }
    return 1;
}

/*
int SpiDrv::waitResponse(uint8_t cmd, tParam* params, uint8_t* numParamRead, uint8_t maxNumParams)
{
    char _data = 0;
    int i =0, ii = 0;

    IF_CHECK_START_CMD(_data)
    {
        CHECK_DATA(cmd | REPLY_FLAG, _data){};

        uint8_t numParam = readChar();

        if (numParam > maxNumParams)
        {
            numParam = maxNumParams;
        }
        *numParamRead = numParam;
        if (numParam != 0)
        {
            for (i=0; i<numParam; ++i)
            {
                params[i].paramLen = readParamLen8();

                for (ii=0; ii<params[i].paramLen; ++ii)
                {
                    // Get Params data
                    params[i].param[ii] = spiTransfer(DUMMY_DATA);
                } 
            }
        } else
        {
            WARN("Error numParams == 0");
            Serial.println(cmd, 16);
            return 0;
        }
        readAndCheckChar(END_CMD, &_data);
    }         
    return 1;
}
*/

int SpiDrv::waitResponse(uint8_t cmd, uint8_t *numParamRead, uint8_t **params, uint8_t maxNumParams)
{
    char _data = 0;
    int i = 0, ii = 0;

    char *index[WL_SSID_MAX_LENGTH];

    for (i = 0; i < WL_NETWORKS_LIST_MAXNUM; i++)
        index[i] = (char *)params + WL_SSID_MAX_LENGTH * i;

    IF_CHECK_START_CMD(_data)
    {
        CHECK_DATA(cmd | REPLY_FLAG, _data){};

        uint8_t numParam = readChar();

        if (numParam > maxNumParams)
        {
            numParam = maxNumParams;
        }
        *numParamRead = numParam;
        if (numParam != 0)
        {
            for (i = 0; i < numParam; ++i)
            {
                uint8_t paramLen = readParamLen8();
                for (ii = 0; ii < paramLen; ++ii)
                {
                    //ssid[ii] = spiTransfer(DUMMY_DATA);
                    // Get Params data
                    index[i][ii] = (uint8_t)spiTransfer(DUMMY_DATA);
                }
                index[i][ii] = 0;
            }
        }
        else
        {
            WARN("Error numParams == 0");
            readAndCheckChar(END_CMD, &_data);
            return 0;
        }
        readAndCheckChar(END_CMD, &_data);
    }
    return 1;
}

void SpiDrv::sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam)
{
    int i = 0;
    // Send Spi paramLen
    sendParamLen8(param_len);

    // Send Spi param data
    for (i = 0; i < param_len; ++i)
    {
        spiTransfer(param[i]);
    }

    // if lastParam==1 Send Spi END CMD
    if (lastParam == 1)
        spiTransfer(END_CMD);
}

void SpiDrv::sendParamLen8(uint8_t param_len)
{
    // Send Spi paramLen
    spiTransfer(param_len);
}

void SpiDrv::sendParamLen16(uint16_t param_len)
{
    // Send Spi paramLen
    spiTransfer((uint8_t)((param_len & 0xff00) >> 8));
    spiTransfer((uint8_t)(param_len & 0xff));
}

uint8_t SpiDrv::readParamLen8(uint8_t *param_len)
{
    uint8_t _param_len = spiTransfer(DUMMY_DATA);
    if (param_len != NULL)
    {
        *param_len = _param_len;
    }
    return _param_len;
}

uint16_t SpiDrv::readParamLen16(uint16_t *param_len)
{
    uint16_t _param_len = spiTransfer(DUMMY_DATA) << 8 | (spiTransfer(DUMMY_DATA) & 0xff);
    if (param_len != NULL)
    {
        *param_len = _param_len;
    }
    return _param_len;
}

void SpiDrv::sendBuffer(uint8_t *param, uint16_t param_len, uint8_t lastParam)
{
    uint16_t i = 0;

    // Send Spi paramLen
    sendParamLen16(param_len);

    // Send Spi param data
    for (i = 0; i < param_len; ++i)
    {
        spiTransfer(param[i]);
    }

    // if lastParam==1 Send Spi END CMD
    if (lastParam == 1)
        spiTransfer(END_CMD);
}

void SpiDrv::sendParam(uint16_t param, uint8_t lastParam)
{
    // Send Spi paramLen
    sendParamLen8(2);

    spiTransfer((uint8_t)((param & 0xff00) >> 8));
    spiTransfer((uint8_t)(param & 0xff));

    // if lastParam==1 Send Spi END CMD
    if (lastParam == 1)
        spiTransfer(END_CMD);
}

/* Cmd Struct Message */
/* _________________________________________________________________________________  */
/*| START CMD | C/R  | CMD  |[TOT LEN]| N.PARAM | PARAM LEN | PARAM  | .. | END CMD | */
/*|___________|______|______|_________|_________|___________|________|____|_________| */
/*|   8 bit   | 1bit | 7bit |  8bit   |  8bit   |   8bit    | nbytes | .. |   8bit  | */
/*|___________|______|______|_________|_________|___________|________|____|_________| */

void SpiDrv::sendCmd(uint8_t cmd, uint8_t numParam)
{
    // Send Spi START CMD
    spiTransfer(START_CMD);

    // Send Spi C + cmd
    spiTransfer(cmd & ~(REPLY_FLAG));

    // Send Spi totLen
    //spiTransfer(totLen);

    // Send Spi numParam
    spiTransfer(numParam);

    // If numParam == 0 send END CMD
    if (numParam == 0)
        spiTransfer(END_CMD);
}

int SpiDrv::available()
{
    if (pGPIO0 >= 0)
    {
        return (gpio_get(pGPIO0) == 1);
    }
    return true;
}

SpiDrv spiDrv;

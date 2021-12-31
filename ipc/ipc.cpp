/********************************************************
 * ipc.cpp
 ********************************************************
 * Pass data and commands between the cores of the 
 * processor.  The silicon provides a pair of 32-bit
 * FIFOs for this, but I wanted to handle more complex
 * data and not be blocking.
 * 
 * December 2021, M.Brugman
 * 
 *******************************************************/

#include "ipc.h"
#include "pico/mutex.h"
#include "../utils/stringFormat.h"
#include "mlogger.h"

// the data structure is copied here, probably unnecessary, but
// provides reliable data integrity
static inter_core_t icData;
static mutex_t mtx;
static logger* log = NULL;

/*******************************************************
 * copyIPC()
 *******************************************************
 * copy one structure to another.  Using some STL 
 * strings and std::vectors, so don't do a std::memcpy()
 ******************************************************/
void copyIPC(const inter_core_t src, inter_core_t& dst)
{
    dst.core1Ready      = src.core1Ready;
    dst.wifiConnected   = src.wifiConnected;
    dst.clockReady      = src.clockReady;
    dst.temperatue      = src.temperatue;
    dst.tempCount       = src.tempCount;
    dst.ipAddress       = src.ipAddress;
    dst.macAddress      = src.macAddress;
    dst.scanResult      = src.scanResult;
    dst.cmd             = src.cmd;
    dst.ack             = src.ack;
}

/*******************************************************
 * initIPC()
 *******************************************************
 * initialize the IPC chingus, including init'ing the
 * mutex
 ******************************************************/
bool initIPC(void)
{
    icData.core1Ready = false;
    icData.wifiConnected = false;
    icData.clockReady = false;
    icData.temperatue = 0.0;
    icData.tempCount = 0;
    icData.cmd = IS_NO_CMD;
    icData.ack = IS_NO_CMD;

    mutex_init(&mtx);

    log = logger::getInstance();

    return (mutex_is_initialized(&mtx));
}

/*******************************************************
 * updateSharedData()
 *******************************************************
 * called by both cores, wrapped in a blocking mutex.
 * 
 * The first parameter is bitmask of which fields have
 * changed
 ******************************************************/
bool updateSharedData(uint16_t t, inter_core_t& d)
{
    bool ret = false;

    if (mutex_is_initialized(&mtx))
    {
        mutex_enter_blocking(&mtx);

        // copy the changed data in
        if (t & US_CORE1_READY)         icData.core1Ready = d.core1Ready;
        if (t & US_WIFI_CONNECTED)      icData.clockReady = d.wifiConnected;
        if (t & US_CLOCK_READY)         icData.clockReady = d.clockReady;
        if (t & US_IP_ADDR)             icData.ipAddress = d.ipAddress;
        if (t & US_MAC_ADDR)            icData.macAddress = d.macAddress;
        if (t & US_SCAN_DATA)           icData.scanResult = d.scanResult;
        if (t & US_NEW_CMD)             icData.cmd = d.cmd;
        if (t & US_ACK_CMD)             icData.ack = d.ack;
        if (t & US_NEW_TMP_DATA)
        {
            icData.temperatue = d.temperatue;
            icData.tempCount = d.tempCount;
        }

        // now copy out from other core
        copyIPC(icData, d);
        ret = true;

        mutex_exit(&mtx);
    }

    return (ret);
}

/*******************************************************
 * cmd2text()
 *******************************************************
 * convenience method to display a command in human
 * readable form
 ******************************************************/
const std::string cmd2text(inter_core_cmd_t c)
{
    switch (c)
    {
        case IS_NO_CMD:             return("No CMD");       break;
        case IS_GET_IP:             return("Get IP Addr");  break;
        case IS_GET_MAC:            return("Get Mac Addr"); break;
        case IS_DO_SCAN:            return("Scan Network"); break;
    }

    return ("WTF?");
}

/*******************************************************
 * dumpStruct()
 *******************************************************
 * convenience method to display a the entire IPC struct
 * in human readable form
 ******************************************************/
void dumpStruct(const inter_core_t& d, const std::string w)
{
    log->dbgWrite(stringFormat("%s::%s\n", __FUNCTION__, w.c_str()));
    log->dbgWrite(stringFormat("  core1Ready: %s\n", d.core1Ready ? "true" : "false"));
    log->dbgWrite(stringFormat("  wifiConnected: %s\n", d.wifiConnected ? "true" : "false"));
    log->dbgWrite(stringFormat("  clockReady: %s\n", d.clockReady ? "true" : "false"));
    log->dbgWrite(stringFormat("  ipAddress: %s\n", d.ipAddress.c_str()));
    log->dbgWrite(stringFormat("  macAddress: %s\n", d.macAddress.c_str()));
    log->dbgWrite(stringFormat("  temperature: %2.1\n", d.temperatue));
    log->dbgWrite(stringFormat("  temperature count %d\n", d.tempCount));
    log->dbgWrite(stringFormat("  cmd: %s\n", cmd2text(d.cmd).c_str()));
    log->dbgWrite(stringFormat("  ack: %s\n", cmd2text(d.ack).c_str()));
}

/*******************************************************
 * diffStruct()
 *******************************************************
 * convenience method to display the differences between
 * two structs
 ******************************************************/
void diffStruct(const inter_core_t& a, const inter_core_t& b, int core)
{
    if (a.core1Ready != b.core1Ready)       log->dbgWrite(stringFormat("%d-->core1Ready from %s to %s\n", core, a.core1Ready? "true" : "false", b.core1Ready? "true" : "false"));
    if (a.wifiConnected != b.wifiConnected) log->dbgWrite(stringFormat("%d-->wifiConnected from %s to %s\n", core, a.wifiConnected? "true" : "false", b.wifiConnected? "true" : "false"));
    if (a.clockReady != b.clockReady)       log->dbgWrite(stringFormat("%d-->clockReady from %s to %s\n", core, a.clockReady? "true" : "false", b.clockReady? "true" : "false"));
    if (a.ipAddress != b.ipAddress)         log->dbgWrite(stringFormat("%d-->ip addr from %s to %s\n", core, a.ipAddress.c_str(), b.ipAddress.c_str()));
    if (a.macAddress != b.macAddress)       log->dbgWrite(stringFormat("%d-->mac addr from %s to %s\n", core, a.macAddress.c_str(), b.macAddress.c_str()));
    if (a.cmd != b.cmd)                     log->dbgWrite(stringFormat("%d-->cmd from %s to %s\n", core, cmd2text(a.cmd).c_str(), cmd2text(b.cmd).c_str()));
    if (a.ack != b.ack)                     log->dbgWrite(stringFormat("%d-->ack from %s to %s\n", core, cmd2text(a.ack).c_str(), cmd2text(b.ack).c_str()));
}
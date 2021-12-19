#include "ipc.h"
#include "pico/mutex.h"
#include "../utils/stringFormat.h"
#include "mlogger.h"


static inter_core_t icData;
static mutex_t mtx;

void copyIPC(const inter_core_t src, inter_core_t& dst)
{
    dst.core1Ready      = src.core1Ready;
    dst.wifiConnected   = src.wifiConnected;
    dst.clockReady      = src.clockReady;
    dst.temperatue      = src.temperatue;
    dst.tempCount       = src.tempCount;
    dst.clockTime       = src.clockTime;
    dst.ipAddress       = src.ipAddress;
    dst.macAddress      = src.macAddress;
    dst.scanResult      = src.scanResult;
    dst.cmd             = src.cmd;
    dst.ack             = src.ack;
}

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

    return (mutex_is_initialized(&mtx));
}

bool updateSharedData(uint16_t t, inter_core_t& d)
{
    bool ret = false;

    if (mutex_is_initialized(&mtx))
    {
        mutex_enter_blocking(&mtx);

        if (t & US_CORE1_READY)         icData.core1Ready = d.core1Ready;
        if (t & US_WIFI_CONNECTED)      icData.clockReady = d.wifiConnected;
        if (t & US_CLOCK_READY)         icData.clockReady = d.clockReady;
        if (t & US_CLOCK_TIME)          std::memcpy(&icData.clockTime, &d.clockTime, sizeof(datetime_t));
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

        copyIPC(icData, d);
        ret = true;

        mutex_exit(&mtx);
    }

    return (ret);
}


const std::string cmd2text(inter_core_cmd_t c)
{
    switch (c)
    {
        case IS_NO_CMD:             return("No CMD");       break;
        case IS_GET_CLOCK_TIME:     return("Get Clock");    break;
        case IS_GET_IP:             return("Get IP Addr");  break;
        case IS_GET_MAC:            return("Get Mac Addr"); break;
        case IS_DO_SCAN:            return("Scan Network"); break;
    }

    return ("WTF?");
}

void dumpStruct(const inter_core_t& d, const std::string w)
{
    dbgWrite(stringFormat("%s::%s\n", __FUNCTION__, w.c_str()));
    dbgWrite(stringFormat("  core1Ready: %s\n", d.core1Ready ? "true" : "false"));
    dbgWrite(stringFormat("  wifiConnected: %s\n", d.wifiConnected ? "true" : "false"));
    dbgWrite(stringFormat("  clockReady: %s\n", d.clockReady ? "true" : "false"));
    dbgWrite(stringFormat("  ipAddress: %s\n", d.ipAddress.c_str()));
    dbgWrite(stringFormat("  macAddress: %s\n", d.macAddress.c_str()));
    dbgWrite(stringFormat("  temperature: %2.1\n", d.temperatue));
    dbgWrite(stringFormat("  temperature count %d\n", d.tempCount));
    dbgWrite(stringFormat("  cmd: %s\n", cmd2text(d.cmd).c_str()));
    dbgWrite(stringFormat("  ack: %s\n", cmd2text(d.ack).c_str()));
}


void diffStruct(const inter_core_t& a, const inter_core_t& b, int core)
{
    if (a.core1Ready != b.core1Ready)   dbgWrite(stringFormat("%d-->core1Ready from %s to %s\n", core, a.core1Ready? "true" : "false", b.core1Ready? "true" : "false"));
    if (a.wifiConnected != b.wifiConnected)   dbgWrite(stringFormat("%d-->wifiConnected from %s to %s\n", core, a.wifiConnected? "true" : "false", b.wifiConnected? "true" : "false"));
    if (a.clockReady != b.clockReady)   dbgWrite(stringFormat("%d-->clockReady from %s to %s\n", core, a.clockReady? "true" : "false", b.clockReady? "true" : "false"));
    if (a.ipAddress != b.ipAddress)     dbgWrite(stringFormat("%d-->ip addr from %s to %s\n", core, a.ipAddress.c_str(), b.ipAddress.c_str()));
    if (a.macAddress != b.macAddress)   dbgWrite(stringFormat("%d-->mac addr from %s to %s\n", core, a.macAddress.c_str(), b.macAddress.c_str()));
    if (a.cmd != b.cmd) dbgWrite(stringFormat("%d-->cmd from %s to %s\n", core, cmd2text(a.cmd).c_str(), cmd2text(b.cmd).c_str()));
    if (a.ack != b.ack) dbgWrite(stringFormat("%d-->ack from %s to %s\n", core, cmd2text(a.ack).c_str(), cmd2text(b.ack).c_str()));
}
/********************************************************
 * ipc.h
 ********************************************************
 * Pass data and commands between the cores of the 
 * processor.  The silicon provides a pair of 32-bit
 * FIFOs for this, but I wanted to handle more complex
 * data and not be blocking.
 * 
 * December 2021, M.Brugman
 * 
 *******************************************************
 * TODO - turn this into yet another singleton class
 *******************************************************/
#ifndef IPC_H_
#define IPC_H_

#include <string>
#include <ctime>
#include <vector>

#include "pico/mutex.h"
#include "pico/multicore.h"

#include "../pilznet/pilznet.h"     // for scan data results

// Commands that core 0 can send to core 1
enum inter_core_cmd_t
{
    IS_NO_CMD = 0,      // null command
    IS_GET_IP,          // get IP address from net
    IS_GET_MAC,         // get MAC address from net
    IS_DO_SCAN          // do a network scan for Access Points
};

// Bitmask of changed data
#define US_NONE             (uint16_t)0x0000        // no changes
#define US_CORE1_READY      (uint16_t)0x0001        // update core 1 ready
#define US_WIFI_CONNECTED   (uint16_t)0x0002        // update wifi connected
#define US_CLOCK_READY      (uint16_t)0x0004        // update clock ready
#define US_IP_ADDR          (uint16_t)0x0008        // update IP address
#define US_MAC_ADDR         (uint16_t)0x0010        // update MAC address
#define US_SCAN_DATA        (uint16_t)0x0020        // new scan data
#define US_NEW_TMP_DATA     (uint16_t)0x0040        // new temperature data
#define US_NEW_CMD          (uint16_t)0x0080        // new command
#define US_ACK_CMD          (uint16_t)0x0100        // new ack

// The main struct to hold data between cores
struct inter_core_t
{
    bool                core1Ready;     // Core 1 is running
    bool                wifiConnected;  // wifi is connected
    bool                clockReady;     // clock has been set
    uint32_t            tempCount;      // incremented each time a temp is written
    float               temperatue;     // current probe reading
    std::string         ipAddress;      // current IP address
    std::string         macAddress;     // current MAC address
    scan_data_t         scanResult;     // results of AP scan
    inter_core_cmd_t    cmd;            // command request between cores
    inter_core_cmd_t    ack;            // command ack
};

// global functions
bool initIPC(void);
void copyIPC(const inter_core_t src, inter_core_t& dst);
bool updateSharedData(uint16_t t, inter_core_t& d);
void dumpStruct(const inter_core_t& d, const std::string w);
void diffStruct(const inter_core_t& a, const inter_core_t& b, int core);

# endif // IPC_H_

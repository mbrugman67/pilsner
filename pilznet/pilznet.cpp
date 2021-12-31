/********************************************************
 * pilznet.h
 ********************************************************
 * Wrapper class for Adafruit Airlift Wifi module.  Uses
 * modified Aruduino libraries from adafruit.com
 * December 2021, M.Brugman
 * 
 *******************************************************/
#include "pilznet.h"
#include "pico/bootrom.h"

#include "../project.h"
#include "../af/Wifi.h"
#include "../utils/stringFormat.h"
#include "../ipc/mlogger.h"
#include "hardware/watchdog.h"

static WiFiClass wifi;          // From Arduino libraries - General Wifi
static WiFiUDP udp;             // From Arduino libraries - UDP server
static logger* log = NULL;

/*******************************************************
 * init()
 *******************************************************
 * Set it up, including SPI pins
 ******************************************************/
void pilznet::init()
{
    // setup SPI for Wifi module
    wifi.setPins(PIN_MOSI, PIN_MISO, PIN_CLOCK, PIN_CS, PIN_READY, PIN_RESET, PIN_GPIO0);
    this->connected = false;
    this->clockValid = false;
    this->macAddr = std::string("00:00:00:00:00:00:");
    this->ipAddr = std::string("0.0.0.0");

    log = logger::getInstance();
}

/*******************************************************
 * connect()
 *******************************************************
 * Attempt to connect to the desired access point
 ******************************************************/
bool pilznet::connect(const std::string& ap, const std::string& pw)
{
    uint8_t timeout = 0;
    this->connected = false;

    // wait for the module to wake up; give up after 10 seconds
    while (WiFi.status() == WL_NO_MODULE)
    {
        if ((++timeout) > 10)
        {
            log->errWrite(stringFormat("Timed out waiting for module to wake up\n"));
            return (false);
        }
        sleep_ms(1000);
    }

    // do the connect
    int conResult = wifi.begin(ap.c_str(), pw.c_str());
    log->dbgWrite(stringFormat("%s::Connection result %s\n", __FUNCTION__, this->status2text(conResult).c_str()));

    if (conResult == WL_CONNECTED)
    {
        this->connected = true;

        // Start the UDP server
        udp.begin(1234);
    }

    // Save the IP and MAC addresses
    this->ipAddr = wifi.localIP().ipToString();
    uint8_t mac[6];
    WiFi.macAddress(mac);
    this->macAddr = mac2text(mac);

    return (this->connected);
}

/*******************************************************
 * update()
 *******************************************************
 * Service the UDP server
 ******************************************************/
bool pilznet::update(void)
{
    bool retVal = false;

    // this will return -1 or the number of bytes 
    // waiting to be pulled.  Any bytes left over from
    // the last check will be discarded
    int bytesAvailable = udp.parsePacket();
    
    // are there some there?
    if (bytesAvailable > 0)
    {
        // we only care about the first character
        char cmd = (char)udp.read();
        switch (cmd)
        {
            // request for log buffer, send it back
            case 'x':
            {
                std::string dbgBuffer = log->dbgPop();
                if (dbgBuffer.length())
                {
                    udp.beginPacket(udp.remoteIP(), udp.remotePort());
                    udp.write((uint8_t*)dbgBuffer.c_str(), dbgBuffer.length());
                    udp.endPacket();
                }
            }  break;

            // request for rebooten 
            case 'n':
            {
                watchdog_enable(1, 0);
                while(true);
            }  break;

            // request for rebooten into UF2 bootloader (save on USB connector cycles!)
            case 'r':
            {
                reset_usb_boot(0, 0);
            }  break;
            
            default:
            {
                log->warnWrite(stringFormat("Saw a '%c' on listening port\n", cmd));
            }
        }
        retVal = true;
    }

    return (retVal);
}

/*******************************************************
 * doNTP()
 *******************************************************
 * Request the current time/date from interwebz using
 * the NTP protocol
 ******************************************************/
bool pilznet::doNTP(const std::string& tz)
{
    this->clockValid = false;

    if (this->connected)
    {
        wt.setTimezone(std::string(tz));
        wt.doNTP();
        sleep_ms(10);
        this->clockValid = true;
    }
    
    return (this->clockValid);
}

/*******************************************************
 * scan()
 *******************************************************
 * Scan for all access points seen by the wifi module
 ******************************************************/
const scan_data_t pilznet::scan()
{
    scan_data_t ret;

    // the module tells us how many networks were seen, will
    // return -1 in error
    ret.count = wifi.scanNetworks();

    if (ret.count == -1)
    {
        log->errWrite(stringFormat("Unable to get a network link\n"));
    }
    else
    {
        // Binary SSID (AP MAC address)
        uint8_t bssid[16];

        for (int ii = 0; ii < ret.count; ++ii)
        {
            ap_data_t d;
            
            // get the BSSID
            wifi.BSSID(ii, bssid);
            // convert to a string for easier handling
            d.bssid = mac2text(bssid);

            // human readable name of the network
            d.ssid = wifi.SSID(ii);

            // strength is DB (will be a negative number)
            d.strength = wifi.RSSI(ii);

            // channel
            d.channel = wifi.channel(ii);

            // encryption type - decode
            d.encryption = encryption2text(wifi.encryptionType(ii));

            ret.apData.push_back(d);
        }
    }

    return (ret);
}

/*******************************************************
 * encryption2text()
 *******************************************************
 * Convenience method to return encryption type as a 
 * human readable string
 ******************************************************/
const std::string pilznet::encryption2text(int thisType) 
{
    switch (thisType) 
    {
        case ENC_TYPE_WEP:      return ("WEP");         break;
        case ENC_TYPE_TKIP:     return ("WPA");         break;
        case ENC_TYPE_CCMP:     return ("WPA2");        break;
        case ENC_TYPE_NONE:     return ("NONE");        break;
        case ENC_TYPE_AUTO:     return ("AUTO");        break;
        case ENC_TYPE_UNKNOWN:  return ("UNKNOWN");     break;
        default:                return ("UNDEFINED");   break;
    }

    return ("WTF");
}

/*******************************************************
 * status2text()
 *******************************************************
 * Convenience method to return module status as a 
 * human readable string
 ******************************************************/
const std::string pilznet::status2text(int status) 
{
    switch (status)
    {
        case WL_IDLE_STATUS:       return ("idle");
        case WL_NO_SSID_AVAIL:     return ("no SSID available");
        case WL_SCAN_COMPLETED:    return ("scan completed");
        case WL_CONNECTED:         return ("connected");
        case WL_CONNECT_FAILED:    return ("connect failed");
        case WL_CONNECTION_LOST:   return ("connection lost");
        case WL_DISCONNECTED:      return ("disconnected");
        case WL_AP_LISTENING:      return ("AP listening");
        case WL_AP_CONNECTED:      return ("AP connected");
        case WL_AP_FAILED:         return ("AP failed");
    }

    return ("unknown");
}

/*******************************************************
 * mac2text()
 *******************************************************
 * Convenience method to return MAC address as a 
 * human readable string
 ******************************************************/
const std::string pilznet::mac2text(uint8_t* mac)
{
    std::string ret;

    if (mac)
    {
        ret = stringFormat("%02x:%02x:%02x:%02x:%02x:%02x",
            mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    }

    return (ret);
}

#include "pilznet.h"
#include "../project.h"

#include "../af/Wifi.h"

static WiFiClass wifi;
static WiFiUDP udp;

void pilznet::init()
{
    // setup SPI for Wifi module
    wifi.setPins(PIN_MOSI, PIN_MISO, PIN_CLOCK, PIN_CS, PIN_READY, PIN_RESET, PIN_GPIO0);
    this->connected = false;
    this->clockValid = false;
    this->macAddr = std::string("00:00:00:00:00:00:");
    this->ipAddr = std::string("0.0.0.0");
}

bool pilznet::connect(const std::string& ap, const std::string& pw)
{
    uint8_t timeout = 0;
    this->connected = false;

    while (WiFi.status() == WL_NO_MODULE)
    {
        if ((++timeout) > 10)
        {
            printf("Timed out waiting for module to wake up\n");
            return (false);
        }
        sleep_ms(1000);
    }

    int conResult = wifi.begin(ap.c_str(), pw.c_str());

    if (conResult == WL_CONNECTED)
    {
        this->connected = true;

        udp.begin(1234);
    }

    this->ipAddr = wifi.localIP().ipToString();
    uint8_t mac[6];
    WiFi.macAddress(mac);
    this->macAddr = mac2text(mac);

    printf("Connection result: '%s'\n", status2text(conResult));
    printf("Local IP address %s\n", wifi.localIP().ipToString().c_str());

    return (this->connected);
}

bool pilznet::update(void)
{
    bool retVal = false;

    int bytesAvailable = udp.parsePacket();
    
    if (bytesAvailable)
    {
        char* buff = new char[bytesAvailable];
        if (!buff)
        {
            printf("Unable to allocate %d bytes for new UDP packet\n", bytesAvailable);
            return (retVal);
        }

        IPAddress remoteIP = udp.remoteIP();
        int remotePort = udp.remotePort();

        int bytesRead = udp.read(buff, bytesAvailable);
        printf("Read %d/%d from remote %s/%d\n", 
            bytesRead, bytesAvailable,
            remoteIP.ipToString().c_str(), remotePort);

        udp.beginPacket(remoteIP, remotePort);
        udp.write((uint8_t*)buff, bytesRead);
        
        if (udp.endPacket())
        {
            printf("Returned packet\n");
        }
        else
        {
            printf("Error returning packet\n");
        }

        retVal = true;
    }


    return (retVal);
}


bool pilznet::doNTP(const std::string& tz)
{
    if (this->connected)
    {
        wt.setTimezone(std::string(tz));
        wt.doNTP();
        sleep_ms(1000);
        this->clockValid = true;

        return (true);
    }
    
    return (false);
}

const std::string pilznet::getTimeString(void)
{
    if (this->clockValid)
    {
        return (wt.timeString());
    }
    return (std::string("Time not set"));
}
const std::string pilznet::getDateString(void)
{
    if (this->clockValid)
    {
        return (wt.dateString());
    }
    return (std::string("Date not set"));
}


/****************************************************
 * Similar to Linux `iwlist wlan0 scanning`
 ****************************************************/
const std::string pilznet::scan()
{
    printf("******  SCANNING ******\n");
    int netCount = wifi.scanNetworks();

    if (netCount == -1)
    {
        printf("Unable to get a network link\n");
    }
    else
    {
        uint8_t bssid[16];

        printf("%d networks found\n", netCount);

        for (int ii = 0; ii < netCount; ++ii)
        {
            printf("Network %d: %s\n", ii, wifi.SSID(ii));

            wifi.BSSID(ii, bssid);
            printf("\tBSSID %s\n", mac2text(bssid));

            printf("\tSignal: %d dbm\n", wifi.RSSI(ii));
            printf("\tChannel: %d\n", (uint16_t)(wifi.channel(ii)));
            printf("\tEncryption: %s\n", encryption2text(wifi.encryptionType(ii)));
        }
    }

    return ("Done");
}


const char* pilznet::encryption2text(int thisType) 
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

const char* pilznet::status2text(int status) 
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

const char* pilznet::mac2text(uint8_t* mac)
{
    static char ret[24] = {0};

    if (mac)
    {
        snprintf(ret, 23, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    }

    return (ret);
}
#include "pilznet.h"

#include "../project.h"
#include "../af/Wifi.h"
#include "../utils/stringFormat.h"

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
    printf("%s::Connection result %s\n", __FUNCTION__, this->status2text(conResult).c_str());

    if (conResult == WL_CONNECTED)
    {
        this->connected = true;

        udp.begin(1234);
    }

    this->ipAddr = wifi.localIP().ipToString();
    uint8_t mac[6];
    WiFi.macAddress(mac);
    this->macAddr = mac2text(mac);

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
    this->clockValid = false;

    if (this->connected)
    {
        wt.setTimezone(std::string(tz));
        wt.doNTP();
        sleep_ms(1000);
        this->clockValid = true;
    }
    
    return (this->clockValid);
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
const scan_data_t pilznet::scan()
{
    scan_data_t ret;

    ret.count = wifi.scanNetworks();

    if (ret.count == -1)
    {
        printf("Unable to get a network link\n");
    }
    else
    {
        uint8_t bssid[16];

        for (int ii = 0; ii < ret.count; ++ii)
        {
            ap_data_t d;

            wifi.BSSID(ii, bssid);
            d.bssid = mac2text(bssid);

            d.ssid = wifi.SSID(ii);
            d.strength = wifi.RSSI(ii);
            d.channel = wifi.channel(ii);
            d.encryption = encryption2text(wifi.encryptionType(ii));

            ret.apData.push_back(d);
        }
    }

    return (ret);
}




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

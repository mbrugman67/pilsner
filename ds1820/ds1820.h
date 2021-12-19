/***************************************************
 * ds1820
 ***************************************************
 * Class to read from a DS1820 temperature probe
 * over 1-wire.  Uses PIO
 * 
 * This based on code found at
 *  https://www.i-programmer.info/programming/hardware/14527-the-pico-in-c-a-1-wire-pio-program.html
 * with a fair bit if cleanup.  
 * 
 * Encapsulated in a c++ class, got rid of all
 * of the naked arrays (ugh - arrays are EVIL!)
 * and added some comments and stuff.
***************************************************/

#ifndef DS_1820_
#define DS_1820_

#include "ds1820.pio.h"

#define BAD_TEMPERATURE_VALUE   -2000

class ds1820
{
public:
    ds1820() {}
    ~ds1820() {}

    uint32_t init(PIO p, int pin);
    float getTemperature() const;

private:
    uint32_t sm;
    PIO pio;

    std::vector<uint8_t> first;
    std::vector<uint8_t> second;

    uint8_t crc8(const std::vector<uint8_t>& data) const;
    void writeBytes(const std::vector<uint8_t>& data) const;
    void readBytes(std::vector<uint8_t>& data, size_t len) const;
};


#endif // DS_1820_
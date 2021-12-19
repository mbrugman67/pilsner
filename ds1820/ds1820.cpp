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

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include <cstdint>
#include <vector>

#include "ds1820.h"

/**************************************************
 * init()
 **************************************************
 * initialize the thing
 * 
 * Parameters:
 *  p - a reference to the PIO instance to use
 *  pin - GPIO pin that the sensor is hooked up to
 * 
 * Returns:
 *  the SM used
 *************************************************/
uint32_t ds1820::init(PIO p, int pin)
{
    first.push_back(0xcc);
    first.push_back(0x44);

    second.push_back(0xCC);
    second.push_back(0xbe);

    this->pio = p;

    uint offset = pio_add_program(this->pio, &DS1820_program);
    this->sm = pio_claim_unused_sm(this->pio, true);
    pio_gpio_init(this->pio, pin);

    pio_sm_config c = DS1820_program_get_default_config(offset);

    sm_config_set_clkdiv_int_frac(&c, 255, 0);
    sm_config_set_set_pins(&c, pin, 1);
    sm_config_set_out_pins(&c, pin, 1);
    sm_config_set_in_pins(&c, pin);
    sm_config_set_in_shift(&c, true, true, 8);

    pio_sm_init(this->pio, this->sm, offset, &c);
    pio_sm_set_enabled(this->pio, this->sm, true);

    return (this->sm);
}

/**************************************************
 * getTemperature()
 **************************************************
 * Read the temperature from the probe
 * 
 * Parameters:
 *  None
 * 
 * Returns:
 *  Current temperature in degrees F
 *************************************************/
float ds1820::getTemperature() const
{
    // send the request
    writeBytes(first);
    sleep_ms(10);
    writeBytes(second);

    // get the response
    std::vector<uint8_t> data;
    this->readBytes(data, 9);

    // verify integrity
    uint8_t crc = this->crc8(data);

    // bad - return a stupid value
    if (crc != 0)
    {
        return (BAD_TEMPERATURE_VALUE);
    }

    // convert to floating point temperature
    int t1 = data[0];
    int t2 = data[1];
    int16_t temp1 = (t2 << 8 | t1);
    float currentTemperature = (float)temp1 / 16;

    // convert from F to C
    currentTemperature *= 1.8;
    currentTemperature += 32.0;

    return (currentTemperature);
}

/**************************************************
 * writeBytes()
 **************************************************
 * Shove bytes out the PIO state machine
 * 
 * Parameters:
 *  data - a std::vector of uint8_t 
 * 
 * Returns:
 *  None
 *************************************************/
void ds1820::writeBytes(const std::vector<uint8_t>& data) const
{
    pio_sm_put_blocking(this->pio, this->sm, 250);
    pio_sm_put_blocking(this->pio, this->sm, data.size() - 1);

    // push each byte into the buffer. 
    for (std::vector<uint8_t>::const_iterator cit = data.begin(); cit != data.end(); ++cit)
    {
         pio_sm_put_blocking(this->pio, this->sm, *cit);
    } 
}

/**************************************************
 * readBytes()
 **************************************************
 * Pull bytes from the PIO state machine
 * 
 * Parameters:
 *  data - a std::vector of uint8_t to hold the 
 *         received data
 *  len - number of bytes to read
 * 
 * Returns:
 *  None
 *************************************************/
void ds1820::readBytes(std::vector<uint8_t>& data, size_t len) const
{
    pio_sm_put_blocking(this->pio, this->sm, 0);
    pio_sm_put_blocking(this->pio, this->sm, len-1);

    // pull each byte from the state machine
    for(size_t ii = 0; ii < len; ++ii)
    {
        data.push_back((uint8_t)(pio_sm_get_blocking(this->pio, this->sm) >> 24));
    } 
}

/**************************************************
 * crc8()
 **************************************************
 * Pull bytes from the PIO state machine
 * 
 * Parameters:
 *  data - a std::vector of uint8_t with data to 
 *         check
 * 
 * Returns:
 *  checksum
 *************************************************/
uint8_t ds1820::crc8(const std::vector<uint8_t>& data) const
{
    uint8_t j;
    uint8_t temp;
    uint8_t databyte;
    uint8_t crc = 0;

    for (std::vector<uint8_t>::const_iterator cit = data.begin(); cit != data.end(); ++cit)
    {
        databyte = *cit;
        for (j = 0; j < 8; j++)
        {
            temp = (crc ^ databyte) & 0x01;
            crc >>= 1;
            if (temp)
                crc ^= 0x8C;
            databyte >>= 1;
        }
    }
    
    return (crc);
}

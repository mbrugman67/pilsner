/********************************************************
 * ir.h
 ********************************************************
 * Infrared remote control handler.  
 * An input pin is connected to an input interrupt 
 * triggering on the falling edge. 
 * 
 * Standard 38KHz format.  Pin is high when idle,
 * sequence stars with a 9ms low start followed by
 * a 4.5ms high header.  Following that are 16
 * pulses to encode 4 8-bit values.  
 * A logical zero is 1.125ms cycle time, and a 
 * logical one is 2.250ms cycle
 * 
 * 8 April 2021, M.Brugman
 * 
 *******************************************************/

#ifndef IR_H_
#define IR_H_

#include <cinttypes>
#include <string>

// union to store the 32-bit IR value.  There are two
// important bytes: the address and command.  There are
// two bytes which are the complement of the addr & cmd
union irData_t
{
    uint32_t raw;
    struct
    {
        uint8_t cmdComp;
        uint8_t cmd;
        uint8_t addrComp;
        uint8_t addr;
    } b;
};

// Enumeration of remote control keys
enum irKey_t
{
    KEY_NULL = 0,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_MENU,
    KEY_OK,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_VOLUP,
    KEY_VOLDN,
    KEY_ENTER,
    KEY_KEYCOUNT
};

class ir
{
public:
    ir(uint8_t pin);
    ~ir() {}

    bool isCodeAvailable();
    irData_t peekRawCode();
    irKey_t getKeyCode();

    bool isKeyNumeric(irKey_t k);
    uint8_t getNumericValue(irKey_t k);
    std::string getKeyName(irKey_t k);

private:
    uint8_t keyMapping[KEY_KEYCOUNT];
};

#endif 
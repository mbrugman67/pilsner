/********************************************************
 * ir.cpp
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

#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

#include "ir.h"

// state machine states for the ISR
#define ISR_IDLE        0       // waiting for start pulse
#define ISR_HDR         1       // measuring the header pulse
#define ISR_DATA        2       // measuring each of the 32 bit data pulses
#define ISR_CHECK       3       // verifying 

#define INTER_KEY_TIME  (uint32_t)(75 * 1000)  // 75ms between hits, no repeat
#define MIN_START_TIME  (uint32_t)(12 * 1000)   // nominal start time 9.0ms, min 7.5ms
#define MAX_START_TIME  (uint32_t)(15 * 1000)   // max 10.5 ms
#define MIN_ZERO_TIME   (uint32_t)(9 * 100)     // nominal bit value 0 time is 1125us
#define MAX_ZERO_TIME   (uint32_t)(13 * 100)    // min 900, max 1300s
#define MIN_ONE_TIME    (uint32_t)(19 * 100)    // nominal bit value 1 time is 1650us
#define MAX_ONE_TIME    (uint32_t)(25 * 100)    // min 1500us, max 1800us

// static volatile bool; ISR sets it high if there's a new 
// good command from the I/R receiver
static volatile bool validCmd = false;

// share between class and ISR
volatile irData_t irData;

static void isrPin(unsigned int pin, long unsigned int event);

/********************************************************
 * Constructor
 ********************************************************
 * Set up the ISR, set the I/R codes to the keymapping
 * array.  In a future version, these values will be 
 * read in from Flash to allow different remote 
 * controllers to work
 *******************************************************/
ir::ir(uint8_t pin)
{   
    gpio_init(pin);
    gpio_set_input_enabled(pin, true);
    gpio_set_pulls(pin, false, false);
    gpio_set_irq_enabled_with_callback (pin, GPIO_IRQ_EDGE_FALL, true, isrPin);

    keyMapping[KEY_NULL] = 0xfe;
    keyMapping[KEY_0] = 0x00;
    keyMapping[KEY_1] = 0x80;
    keyMapping[KEY_2] = 0x40;
    keyMapping[KEY_3] = 0xc0;
    keyMapping[KEY_4] = 0x20;
    keyMapping[KEY_5] = 0xa0;
    keyMapping[KEY_6] = 0x60;
    keyMapping[KEY_7] = 0xe0;
    keyMapping[KEY_8] = 0x10;
    keyMapping[KEY_9] = 0x90;
    keyMapping[KEY_MENU] = 0xc2;
    keyMapping[KEY_OK] = 0xff;
    keyMapping[KEY_UP] = 0x02;
    keyMapping[KEY_DOWN] = 0x7a;
    keyMapping[KEY_LEFT] = 0x42;
    keyMapping[KEY_RIGHT] = 0x82;
    keyMapping[KEY_VOLUP] = 0x50;
    keyMapping[KEY_VOLDN] = 0xd0;
    keyMapping[KEY_ENTER] = 0xd2;
}

/********************************************************
 * isCodeAvailable()
 ********************************************************
 * indicate that there is a good code waiting
 *******************************************************/
bool ir::isCodeAvailable()
{
    return (validCmd);
}

/********************************************************
 * peekRawCode()
 ********************************************************
 * Return the raw I/R data, but do not clear out the 
 * current command
 *******************************************************/
irData_t ir::peekRawCode()
{
    irData_t retval;
    retval.raw = irData.raw;

    return (retval);
}

/********************************************************
 * getKeyCode()
 ********************************************************
 * Return the current keycode if one is waiting to be
 * read.  Clear the "good command waiting" flag along
 * the way
 *******************************************************/
irKey_t ir::getKeyCode()
{
    irKey_t retKey = KEY_NULL;

    if (validCmd)
    {
        validCmd = false;

        for (uint8_t ii = KEY_0; ii < KEY_KEYCOUNT; ++ii)
        {
            if (keyMapping[ii] == irData.b.cmd)
            {
                return ((irKey_t)ii);
            }
        }
    }

    return (retKey);
}

/********************************************************
 * getNumericValue()
 ********************************************************
 * If the supplied keycode command type represents a
 * numeric key, return the integer value; otherwise
 * return KEY_NULL
 *******************************************************/
uint8_t ir::getNumericValue(irKey_t k)
{
    switch (k)
    {
        case KEY_0:             return(0);
        case KEY_1:             return(1);
        case KEY_2:             return(2);
        case KEY_3:             return(3);
        case KEY_4:             return(4);
        case KEY_5:             return(5);
        case KEY_6:             return(6);
        case KEY_7:             return(7);
        case KEY_8:             return(8);
        case KEY_9:             return(9);
    }

    return (keyMapping[KEY_NULL]);
}

/********************************************************
 * isKeyNumeric()
 ********************************************************
 * is the supplied keycode value representative of
 * a numveric value?
 *******************************************************/
bool ir::isKeyNumeric(irKey_t k)
{
    return (this->getNumericValue(k) != keyMapping[KEY_NULL]);
}

/********************************************************
 * getKeyName()
 ********************************************************
 * return a human-readable version of the keycode type
 *******************************************************/
std::string ir::getKeyName(irKey_t k)
{
    switch (k)
    {
        case KEY_NULL:          return(std::string("None"));
        case KEY_0:             return(std::string("0"));
        case KEY_1:             return(std::string("1"));
        case KEY_2:             return(std::string("2"));
        case KEY_3:             return(std::string("3"));
        case KEY_4:             return(std::string("4"));
        case KEY_5:             return(std::string("5"));
        case KEY_6:             return(std::string("6"));
        case KEY_7:             return(std::string("7"));
        case KEY_8:             return(std::string("8"));
        case KEY_9:             return(std::string("9"));
        case KEY_MENU:          return(std::string("Menu"));
        case KEY_OK:            return(std::string("OK"));
        case KEY_UP:            return(std::string("Up"));
        case KEY_DOWN:          return(std::string("Down"));
        case KEY_LEFT:          return(std::string("Left"));
        case KEY_RIGHT:         return(std::string("Right"));
        case KEY_VOLUP:         return(std::string("Volume Up"));
        case KEY_VOLDN:         return(std::string("Volume Down"));
        case KEY_ENTER:         return(std::string("Enter"));
    }

    return (std::string("Invalid key code"));
}

/*******************************************************
 * isrPin
 *******************************************************  
 * Pin interrupt ISR.  Times the pulses, validates,
 * and let's the rest of the class know there's a new
 * value.
 * 
 * The parameters from the NVIC are pin and event; you
 * could use this type of ISR for several pins and/or
 * different edges/levels.  In this case, it is only
 * one pin, and only the falling edge, so we can 
 * ignore them.
 ******************************************************/
static void isrPin(unsigned int pin, long unsigned int event)
{
    static uint32_t lastTime = 0;       // keep track of ticks
    static uint8_t state = ISR_IDLE;    // state machine
    static uint8_t posn = 0;            // bit position

    // we really only care about the delta time.  The hardware
    // timer is 64 bits, but we only care about the lower
    // 32 bits.  That's sufficient for our purposes.  Also,
    // read the raw value so as not to set up the gated
    // low/high read.  The time is in nanoseconds
    uint32_t isrTime = timer_hw->timerawl;

    // elapsed
    uint32_t elapsed = isrTime - lastTime;

    // for next time
    lastTime = isrTime;

    switch (state)
    {
        // Wait for a falling edge.  In the protocol, an 
        // interval of 40ms means this is a "repeat"; i.e.,
        // the key was held.  We want none of that, so we're
        // going to make sure the line was idle for more 
        // like 100 ms.
        case ISR_IDLE:
        {
            // if it's been long enough, set up local
            // variables and let 'er go
            if (elapsed > INTER_KEY_TIME)
            {
                posn = 0;
                irData.raw = 0;
                validCmd = false;
                state = ISR_HDR;
            }

        }  break;

        // The low-to-low cycle time of the very first pulse is around
        // 13.5 ms.  This is the header, and lets us know for sure that
        // we're sync'd to the beginning of the process
        case ISR_HDR:
        {
            if (elapsed > MIN_START_TIME && elapsed < MAX_START_TIME)
            {
                state = ISR_DATA;
            }
            else
            {
                // if not, something goofy is going on, so
                // just go back to idle
                state = ISR_IDLE;
            }
        }  break;

        // The next 32 pulse are the actual data.  The low time of each
        // pulse is about 562.5us.  For a logical zero, the high is 526.5us
        // for a cycle time of 1.125ms.  For a logical zero, the high is
        // 3 * 562.5us for a total cycle time of 2.250ms
        case ISR_DATA:
        {
            // shift left one in anticipation for the next bit
            irData.raw = irData.raw << 1;
            if (elapsed > MIN_ZERO_TIME && elapsed < MAX_ZERO_TIME)
            {
                // mask off low bit for a zero
                irData.raw &= 0xfffffffe;
            }
            else if (elapsed > MIN_ONE_TIME && elapsed < MAX_ONE_TIME)       
            {
                // or by one for logical one
                irData.raw |= 0x00000001;
            }
            else
            {
                // timing is wonked, jump back to idle and wait
                // for the next good cycle.
                state = ISR_IDLE;
            }

            // next bit
            ++posn;

            // Got all 32?
            if (posn == 32)
            {
                state = ISR_CHECK;
            }

        }  break;

        // Check to make sure the command and the complement of the
        // command match
        case ISR_CHECK:
        {
            if (irData.b.cmd  == (irData.b.cmdComp ^ 0xff))
            {
                // if so, this was valid!
                validCmd = true;
            }

            state = ISR_IDLE;
        }  break;
    }
}
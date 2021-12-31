/********************************************************
 * reefer.cpp
 ********************************************************
 * Handle the refrigeration functions - compare temperature
 * feedback to setpoint, turn on pump output, etc.
 * December 2021, M.Brugman
 * 
 *******************************************************/

#include "reefer.h"
#include "project.h"

#define INIT_TIME_DELAY         (uint32_t)(30 * 1000)   // 30 seconds for startup delay
#define CHILL_START_DELAY       (uint32_t)(60 * 1000)   // 60 second minimum pump run time
#define CHILL_END_DELAY         (uint32_t)(60 * 1000)   // 60 second minimum pump off time

/*********************************************
 * init()
 ******************************************** 
 * set up initial state, get instance of global
 * logger and data classes
 ********************************************/ 
void reefer::init()
{
    log = logger::getInstance();
    data = nvm::getInstance();

    // initial timestamp reference is 30 seconds from now
    refTimestamp = make_timeout_time_ms(INIT_TIME_DELAY);

    // set up the pin that goes to the reefer pump and
    // make sure we start with the pump off
    gpio_init(PIN_PUMP);
    gpio_set_dir(PIN_PUMP, GPIO_OUT);
    gpio_put(PIN_PUMP, false);

    lastTemp = 0.0;
    reeferState = RS_INIT;
}

/*********************************************
 * update()
 ******************************************** 
 * Call periodically to update the reefer
 * state machine.  One state transition per
 * call
 ********************************************/ 
void reefer::update(float currentTemp)
{
    switch (reeferState)
    {
        // Initial power up state - wait 30 seconds 
        // after powerup before entering the state machine
        case RS_INIT:
        {
            if (get_absolute_time() > refTimestamp)
            {
                log->dbgWrite("reefer leaving init state\n");
                reeferState = RS_IDLE;
            }
        }  break;

        // checking temperature to see if we should 
        // turn on the reefer pump
        case RS_IDLE:
        {
            // if the temperature is above the setpoint, turn the pump on.
            // add some hysteresis to prevent unnecessary cycling
            if ((currentTemp + data->getHysteresis()) > data->getSetpoint())
            {
                gpio_put(PIN_PUMP, true);
                log->dbgWrite(stringFormat("Pump on %02.1/%02.1\n", currentTemp, data->getSetpoint()));

                // pump needs to run a minimum amount of time
                refTimestamp = make_timeout_time_ms(CHILL_START_DELAY);

                // oo to the next state
                reeferState = RS_CHILL_START;
            }
        }  break;

        // wait for the minimum chill time to expire
        case RS_CHILL_START:
        {
            if (get_absolute_time() > refTimestamp)
            {
                reeferState = RS_CHILLING;
            }
        }  break;

        // keep the reefer pump running until the wort is chilled
        case RS_CHILLING:
        {
            // if the temperature is below the setpoint, turn the pump off.
            // add some hysteresis to prevent unnecessary cycling
            if ((currentTemp + data->getHysteresis()) < data->getSetpoint())
            {
                gpio_put(PIN_PUMP, false);
                log->dbgWrite(stringFormat("Pump off %02.1/%02.1\n", currentTemp, data->getSetpoint()));

                // pump needs to be off a minimum amount of time
                refTimestamp = make_timeout_time_ms(CHILL_END_DELAY);

                // oo to the next state
                reeferState = RS_POST_CHILL;
            }
        }  break;

        // off time has expired, go back to idle
        case RS_POST_CHILL:
        {
            if (get_absolute_time() > refTimestamp)
            {
                reeferState = RS_IDLE;
            }
        }  break;
    }

    if (gpio_get(PIN_PUMP))     pumpRunning = true;
    else                        pumpRunning = false;

    static uint32_t lastTempCount = 0;
    static float lastTemp = 0.0;

    // drop a line in the log when the temperature changes, include
    // the pump state
    if (lastTemp != currentTemp)
    {
        log->infoWrite(stringFormat("%02.1f,%s\n", 
                currentTemp, 
                pumpRunning ? "running":"stopped"));
    }
    
    lastTemp = currentTemp;
}

/*********************************************
 * getStateName()
 ******************************************** 
 * Convenience method to return human readable
 * reefer state
 ********************************************/ 
const std::string reefer::getStateName()
{
    switch(reeferState)
    {
        case RS_INIT:           return (std::string("Init"));
        case RS_IDLE:           return (std::string("Idle"));
        case RS_CHILL_START:    return (std::string("Chill Starting"));
        case RS_CHILLING:       return (std::string("Chilling"));
        case RS_POST_CHILL:     return (std::string("Post Chill"));
    }

    return (std::string("Undefined"));
}
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

#define INIT_TIME_DELAY         (uint32_t)(30 * 1000)       // 30 seconds for startup delay
#define CHILL_START_DELAY       (uint32_t)(60 * 1000)       // 60 second minimum pump run time
#define CHILL_END_DELAY         (uint32_t)(60 * 1000)       // 60 second minimum pump off time
#define LOG_WRITE_DELAY         (uint32_t)(15 * 60 * 1000)  // 15 minutes between log writes

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
    logWriteTime = make_timeout_time_ms(LOG_WRITE_DELAY);

    // set up the pin that goes to the reefer pump and
    // make sure we start with the pump off
    gpio_init(PIN_PUMP);
    gpio_set_dir(PIN_PUMP, GPIO_OUT);
    gpio_put(PIN_PUMP, false);

    lastTemp = 0.0;
    pumpRuntimeSeconds = 0;
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
    static uint32_t startTime = 0;
    switch (reeferState)
    {
        // Initial power up state - wait 30 seconds 
        // after powerup before entering the state machine
        case RS_INIT:
        {
            if (time_reached(refTimestamp))
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
                log->dbgWrite(stringFormat("Pump on %02.1f/%02.1f\n", currentTemp, data->getSetpoint()));

                // pump needs to run a minimum amount of time
                refTimestamp = make_timeout_time_ms(CHILL_START_DELAY);

                // start the runtime timer
                startTime =  to_ms_since_boot(get_absolute_time());

                // oo to the next state
                reeferState = RS_CHILL_START;
            }
        }  break;

        // wait for the minimum chill time to expire
        case RS_CHILL_START:
        {
            if (time_reached(refTimestamp))
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
                log->dbgWrite(stringFormat("Pump off %02.1f/%02.1f\n", currentTemp, data->getSetpoint()));

                // pump needs to be off a minimum amount of time
                refTimestamp = make_timeout_time_ms(CHILL_END_DELAY);

                // get accumulated runtime
                pumpRuntimeSeconds = (to_ms_since_boot(get_absolute_time()) - startTime) / 1000;
                log->dbgWrite(stringFormat("Accumulated pump runtime now %d seconds\n", pumpRuntimeSeconds));

                // oo to the next state
                reeferState = RS_POST_CHILL;
            }
        }  break;

        // off time has expired, go back to idle
        case RS_POST_CHILL:
        {
            if (time_reached(refTimestamp))
            {
                reeferState = RS_IDLE;
            }
        }  break;
    }

    if (gpio_get(PIN_PUMP))     pumpRunning = true;
    else                        pumpRunning = false;

    // drop a line in the log every 15 minutes
    if (time_reached(logWriteTime))
    {
        logWriteTime = make_timeout_time_ms(LOG_WRITE_DELAY);

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
const std::string reefer::getStateName(reefer_state_t state)
{
    switch(state)
    {
        case RS_INIT:           return (std::string("Init"));
        case RS_IDLE:           return (std::string("Idle"));
        case RS_CHILL_START:    return (std::string("Chill Starting"));
        case RS_CHILLING:       return (std::string("Chilling"));
        case RS_POST_CHILL:     return (std::string("Post Chill"));
    }

    return (std::string("Undefined"));
}
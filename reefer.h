/********************************************************
 * reefer.h
 ********************************************************
 * Handle the refrigeration functions - compare temperature
 * feedback to setpoint, turn on pump output, etc.
 * December 2021, M.Brugman
 * 
 *******************************************************/

#include "pico/stdlib.h"

#include "project.h"
#include "./ipc/mlogger.h"
#include "./utils/stringFormat.h"
#include "./sys/nvm.h"

// State definition for the reefer state machine
enum reefer_state_t
{
    RS_INIT = 0,            // startup state
    RS_IDLE,                // monitor temperature
    RS_CHILL_START,         // pump started, waiting for chill to start
    RS_CHILLING,            // chilling, waiting for temp to drop
    RS_POST_CHILL           // done chilling, wait before next chill cycle
};

class reefer
{
public:
    reefer() {}
    ~reefer() {}
    
    void init();

    void update(float currentTemp);
    bool isPumpRunning()                { return(pumpRunning); }
    reefer_state_t getReeferState()     { return(reeferState); }
    const std::string state2Text()      { return(getStateName()); }

private:
    bool pumpRunning;
    float lastTemp;
    reefer_state_t reeferState;
    absolute_time_t refTimestamp;

    logger* log;
    nvm* data;

    const std::string getStateName();
};
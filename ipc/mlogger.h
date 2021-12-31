/********************************************************
 * mlogger.h
 ********************************************************
 * common logger, circular buffer with mutexes to handle
 * multicore use
 * December 2021, M.Brugman
 * 
 *******************************************************/
#ifndef MLOGGER_H_
#define MLOGGER_H_

#include <string>
#include "pico/multicore.h"

// there's only 260K of RAM, be reasonable
#define LOGGER_BUFFER_SIZE   2048
class logger
{
public:
    // singleton
    static logger* getInstance();
    
    bool initBuffer();      // none of these 4 
    size_t available();     // will probably be
    size_t consumed();      // called by outside,
    bool isFull();          // but WTF, hey.

    // two methods to pull whatever is in the buffer
    void dbgPop(char* c, size_t& len);  // pull up to len chars in buffer
    std::string dbgPop();               // pull it all as a std::string

    // generic write to buffer
    size_t write(const std::string& s);

    // more specialized
    size_t msgWrite(const std::string& ms);     // prefix with [+] date time
    size_t dbgWrite(const std::string& ds);     // prefix with [D] date time
    size_t infoWrite(const std::string& is);    // prefix with [I] date time
    size_t errWrite(const std::string& es);     // prefix with [E] date time
    size_t warnWrite(const std::string& ws);    // prefix with [W] date time      
    size_t hexWrite(int i);                     // write in 0x00 hex format

private:
    bool isEmpty();
    size_t addChar(char c);
    void dbgPop(char& c);

    char buff[LOGGER_BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t used;
    size_t capacity;
    mutex_t dbgMtx;

    static logger* instance;
};

#endif // MLOGGER_H_
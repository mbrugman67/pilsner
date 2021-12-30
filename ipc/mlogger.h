

#ifndef MLOGGER_H_
#define MLOGGER_H_

#include <string>
#include "pico/multicore.h"

#define LOGGER_BUFFER_SIZE   2048
class logger
{
public:
    static logger* getInstance();
    
    bool initBuffer();
    size_t available();
    size_t consumed();
    bool isFull();

    void dbgPop(char* c, size_t& len);
    std::string dbgPop();

    size_t write(const std::string& s);
    size_t msgWrite(const std::string& ms);
    size_t dbgWrite(const std::string& ds);
    size_t infoWrite(const std::string& is);
    size_t errWrite(const std::string& es);
    size_t warnWrite(const std::string& ws);
    size_t hexWrite(int i);

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
/********************************************************
 * mlogger.cpp
 ********************************************************
 * common logger, circular buffer with mutexes to handle
 * multicore use
 * December 2021, M.Brugman
 * 
 *******************************************************/
#include <string>
#include <cstring>

#include "mlogger.h"
#include "../utils/stringFormat.h"
#include "pico/multicore.h"
#include "../sys/walltime.h"

logger* logger::instance = NULL;

/********************************************************
 * getInstance
 ********************************************************
 * get the single instance of the class, instantiate
 * if necessary.
 * 
 * Note - there is no instance count or deletion - this
 * is embedded, baby; things stay for the duration!
 *******************************************************/
logger* logger::getInstance()
{
    // If this is the first call, instantiate it and
    // do the init method
    if (!instance)
    {
        instance = new logger();
        instance->initBuffer();
    }

    // return the single instance
    return (instance);
}

/*********************************************
 * initDebugBuffer()
 *********************************************
 * initialize the buffer, set the pointers
 ********************************************/
bool logger::initBuffer()
{
    std::memset(buff, 0, LOGGER_BUFFER_SIZE);
    head = 0;
    tail = 0;
    used = 0;
    capacity = LOGGER_BUFFER_SIZE;

    // init the mutex
    mutex_init(&dbgMtx);

    return (true);
}

/*********************************************
 * available()
 *********************************************
 * Returns number of bytes left in buffer
 ********************************************/
size_t logger::available()
{
    return (capacity - ((head - tail) + capacity) % capacity);
}

/*********************************************
 * consumed()
 *********************************************
 * Returns number of bytes used in buffer
 ********************************************/
size_t logger::consumed()
{
    return (capacity - available());
}

/*********************************************
 * isEmpty()
 *********************************************
 * Returns true if buffer empty
 ********************************************/
bool logger::isEmpty()
{
    return (head == tail);
}

/*********************************************
 * isFull()
 *********************************************
 * Returns true if buffer full
 ********************************************/
bool logger::isFull()
{
    return (consumed() == capacity - 1);
}

/*********************************************
 * addChar()
 *********************************************
 * add a single character and return the
 * number of characters added; that is, 1.
 * 
 * Not callable from outside, this is just the
 * local implementation to add a char
 ********************************************/
size_t logger::addChar(char c)
{
    mutex_enter_blocking(&dbgMtx);

    // Never ending buffer... if it's full, just
    // increment the tail by one to make room.
    // this the newest byte will overwrite
    // the oldest
    if (this->isFull())
    {
        ++tail;
        tail %= LOGGER_BUFFER_SIZE;
    }

    // add and increment
    buff[head] = c;
    ++head;
    head %= LOGGER_BUFFER_SIZE;  
    mutex_exit(&dbgMtx);  

    return (1);
}

/*********************************************
 * dbgWrite()
 *********************************************
 * add an STL string to the buffer and return
 * the number of bytes added
 ********************************************/
size_t logger::write(const std::string& s)
{
    size_t count = 0;

    printf(s.c_str());

    // strings are just STL containers, so
    // use the standard tools
    std::string::const_iterator it = s.begin();
    while (it != s.end())
    {
        count += this->addChar(*it);
        ++it;
    }

    return (count);
}

/*********************************************
 * msgWrite()
 *********************************************
 * Add a string with a categorization header
 * and datetime string
 ********************************************/
size_t logger::msgWrite(const std::string& ms)
{
    std::string s = stringFormat("[+%d] %s,%s", get_core_num(), walltime::logTimeString().c_str(), ms.c_str());
    return (this->write(s));
}

/*********************************************
 * dbgWrite()
 *********************************************
 * Add a string with a categorization header
 * and datetime string
 ********************************************/
size_t logger::dbgWrite(const std::string& ms)
{
    std::string s = stringFormat("[D%d] %s,%s", get_core_num(), walltime::logTimeString().c_str(), ms.c_str());
    return (this->write(s));
}

/*********************************************
 * infoWrite()
 *********************************************
 * Add a string with a categorization header
 * and datetime string
 ********************************************/
size_t logger::infoWrite(const std::string& is)
{
    std::string s = stringFormat("[+%d] %s,%s", get_core_num(), walltime::logTimeString().c_str(), is.c_str());
    return (this->write(s));
}

/*********************************************
 * errWrite()
 *********************************************
 * Add a string with a categorization header
 * and datetime string
 ********************************************/
size_t logger::errWrite(const std::string& es)
{
    std::string s = stringFormat("[E%d] %s,%s", get_core_num(), walltime::logTimeString().c_str(), es.c_str());
    return (this->write(s));
}

/*********************************************
 * warnWrite()
 *********************************************
 * Add a string with a categorization header
 * and datetime string
 ********************************************/
size_t logger::warnWrite(const std::string& ws)
{
    std::string s = stringFormat("[W%d] %s,%s", get_core_num(), walltime::logTimeString().c_str(), ws.c_str());
    return (this->write(s));
}

/*********************************************
 * hexWrite()
 *********************************************
 * add an integer, but do it as a hex value
 * in the form of 0x0000 to the buffer and 
 * return the number of bytes added
 ********************************************/
size_t logger::hexWrite(int i)
{
    return (this->write(stringFormat("0x%04x", i)));
}

/*********************************************
 * dbgPop()
 *********************************************
 * pull the oldest character out of the buffer.
 * Internal implementation method
 ********************************************/
void logger::dbgPop(char& c)
{
    if (head != tail)
    {
        mutex_enter_blocking(&dbgMtx);
        c = buff[tail];
        ++tail;
        tail %= LOGGER_BUFFER_SIZE;
        mutex_exit(&dbgMtx);
    }
}

/*********************************************
 * dbgPop()
 *********************************************
 * return zero or more bytes from the buffer
 * max is either caller's buffer size or
 * however many bytes are in the buffer, 
 * whichever is smaller.  Will set the 
 * 'len' parameter from size of caller's 
 * buffer to the number of bytes actually
 * loaded
 ********************************************/
void logger::dbgPop(char* c, size_t& len)
{
    if (c && len > 0)
    {
        if (len > this->consumed())
        {
            len = this->consumed();
        }

        for (size_t ii = 0; ii < len; ++ii)
        {
            this->dbgPop(c[ii]);
        }
    }
}

/*********************************************
 * dbgPop()
 *********************************************
 * return all of the contents of the buffer
 * as a std::string
 ********************************************/
std::string logger::dbgPop()
{
    std::string ret;

    size_t len = this->consumed();
    if (len)
    {
        char c;
        ret.resize(len + 1);
        for (size_t ii = 0; ii < len; ++ii)
        {
            dbgPop(c);
            ret[ii] = c;
        }
    }

    return (ret);
}

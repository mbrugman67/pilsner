#include <string>
#include <cstring>

#include "../utils/stringFormat.h"

#define DEBUG_BUFFER_SIZE   2048

static char buff[DEBUG_BUFFER_SIZE];
static size_t head = 0;
static size_t tail = 0;
static size_t used = 0;
static size_t capacity = DEBUG_BUFFER_SIZE;


void resetDebugBuffer()
{
    std::memset(buff, 0, DEBUG_BUFFER_SIZE);
    head = 0;
    tail = 0;
    used = 0;
    capacity = DEBUG_BUFFER_SIZE;
}

size_t available()
{
    return (capacity - ((head - tail) + capacity) % capacity);
}

size_t consumed()
{
    return (capacity - available());
}

bool isEmpty()
{
    return (head == tail);
}

bool isFull()
{
    return (consumed() == capacity - 1);
}

size_t addChar(char c)
{
    if (isFull())
    {
        ++tail;
        tail %= DEBUG_BUFFER_SIZE;
    }

    buff[head] = c;
    ++head;
    head %= DEBUG_BUFFER_SIZE;    

    return (1);
}

size_t dbgWrite(const std::string& s)
{
    size_t count = 0;

    std::string::const_iterator it = s.begin();
    while (it != s.end())
    {
        count += addChar(*it);
        ++it;
    }

    return (count);
}

size_t dbgWrite(int i)
{
    return (dbgWrite(stringFormat("%d", i)));
}

size_t dbgHexWrite(int i)
{
    return (dbgWrite(stringFormat("0x%04x", i)));
}

bool dbgPop(char& c)
{
    bool ret = false;

    if (head != tail)
    {
        c = buff[tail];
        
        ++tail;
        tail %= DEBUG_BUFFER_SIZE;

        ret = true;
    }

    return (ret);
}

bool dbgPop(char* c, size_t& len)
{
    bool ret = false;
    if (c && len > 0)
    {
        if (len > consumed())
        {
            len = consumed();
        }

        for (size_t ii = 0; ii < len; ++ii)
        {
            dbgPop(c[ii]);
        }
        
        ret = true;
    }

    return (true);
}

std::string dbgPop()
{
    std::string ret;

    size_t len = consumed();
    if (len)
    {
        char c;
        ret.resize(len + 1);
        for (size_t ii = 0; ii < len; ++ii)
        {
            dbgPop(c);
            ret.push_back(c);
        }
    }

    return (ret);
}

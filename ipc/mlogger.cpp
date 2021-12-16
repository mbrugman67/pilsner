#include <string>
#include <cstring>

#include "../utils/stringFormat.h"

#define DEBUG_BUFFER_SIZE   2048

static char* buff = NULL;
static size_t head = 0;
static size_t tail = 0;
static size_t used = 0;
static size_t capacity = DEBUG_BUFFER_SIZE;

/*********************************************
 * initDebugBuffer()
 *********************************************
 * initialize the buffer, allocate, etc.
 * 
 * Returns false if unable to allocate
 ********************************************/
bool initDebugBuffer()
{
    if (buff)
    {
        delete[] buff;
    }

    buff = new char[DEBUG_BUFFER_SIZE];
    if (!buff)
    {
        return (false);
    }

    std::memset(buff, 0, DEBUG_BUFFER_SIZE);
    head = 0;
    tail = 0;
    used = 0;
    capacity = DEBUG_BUFFER_SIZE;

    return (true);
}

/*********************************************
 * available()
 *********************************************
 * Returns number of bytes left in buffer
 ********************************************/
size_t available()
{
    return (capacity - ((head - tail) + capacity) % capacity);
}

/*********************************************
 * consumed()
 *********************************************
 * Returns number of bytes used in buffer
 ********************************************/
size_t consumed()
{
    return (capacity - available());
}

/*********************************************
 * isEmpty()
 *********************************************
 * Returns true if buffer empty
 ********************************************/
bool isEmpty()
{
    return (head == tail);
}

/*********************************************
 * isFull()
 *********************************************
 * Returns true if buffer full
 ********************************************/
bool isFull()
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
size_t addChar(char c)
{
    // Never ending buffer... if it's full, just
    // increment the tail by one to make room.
    // this the newest byte will overwrite
    // the oldest
    if (isFull())
    {
        ++tail;
        tail %= DEBUG_BUFFER_SIZE;
    }

    // add and increment
    buff[head] = c;
    ++head;
    head %= DEBUG_BUFFER_SIZE;    

    return (1);
}

/*********************************************
 * dbgWrite()
 *********************************************
 * add an STL string to the buffer and return
 * the number of bytes added
 ********************************************/
size_t dbgWrite(const std::string& s)
{
    size_t count = 0;

    // strings are just STL containers, so
    // use the standard tools
    std::string::const_iterator it = s.begin();
    while (it != s.end())
    {
        count += addChar(*it);
        ++it;
    }

    return (count);
}

/*********************************************
 * dbgWrite()
 *********************************************
 * add an integer to the buffer and return
 * the number of bytes added
 ********************************************/
size_t dbgWrite(int i)
{
    return (dbgWrite(stringFormat("%d", i)));
}

/*********************************************
 * dbgWrite()
 *********************************************
 * add an integer, but do it as a hex value
 * in the form of 0x0000 to the buffer and 
 * return the number of bytes added
 ********************************************/
size_t dbgHexWrite(int i)
{
    return (dbgWrite(stringFormat("0x%04x", i)));
}

/*********************************************
 * dbgPop()
 *********************************************
 * pull the oldest character out of the buffer.
 * Internal implementation method
 ********************************************/
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

/*********************************************
 * dbgPop()
 *********************************************
 * return all of the contents of the buffer
 * as a std::string
 ********************************************/
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

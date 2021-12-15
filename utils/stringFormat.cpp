#include <cstring>
#include <cstdarg>
#include "stringFormat.h"

// Do formatting like snprintf() but for std::strings 
// BIG NOTE:  YOU CANNOT PASS A STD::STRING as a token replacement; 
// this uses vsnprintf() internally, which doesn't understand std::strings.
//
// Specifically DO NOT do something like this:
//
//  ......
//  std::string someString;
//  someString = "abcdef";
//
//  std::string result = stringFormat("Here's the result: %s\n", someString);
//  .......
//
// This will not give you a result like "Here's the result: abcdef"
// Instead, you'll probably have a runtime exception.  However, this
// would be totes legit:
//
//  std::string result = stringFormat("Here's the result: %s\n", someString.c_str());
//
std::string stringFormat(const std::string& fmt, ...) 
{
    // start with a std::string raw buffer of 1k
    int size = 1024;
    std::string str;
    va_list ap;
    size_t retries = 0;

    while (true) 
    {
        str.resize(size);
        va_start(ap, fmt);

        // try to format print into the raw buvver of the std::string
        ssize_t n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);

        // This means that the std::string was allocated big enough.  Trim
        // the std::string's buffer down and head out
        if (n > -1 && n < size) 
        {
            str.resize(n);
            return (str);
        }

        // A non-negative return value indicates more data would've been 
        // written had there been enough buffer.  So set the buffer size
        // to be one larger than the number needed and go try again.  Note that 
        // the return value includes the trailing null
        if (n > -1)
        {
            size = n + 1;
        }
        // a negative return value indicates an error occurred.  Double the 
        // size of the buffer and try again.  Give up after some arbitrary
        // number of tries
        else
        {
            if (++retries < 5)
            {
                size *= 2;
            }
            else
            {
                return std::string("");
            }
            
        }
    }
    return (str);
}
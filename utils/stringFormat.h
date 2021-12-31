/********************************************************
 * stringFormat.h
 ********************************************************
 * do safe snprintf() type formatting with std::strings
 * December 2021, M.Brugman
 * 
 *******************************************************/
#ifndef STRING_FORMAT_H_
#define STRING_FORMAT_H_

#include <string>

std::string stringFormat(const std::string& fmt, ...);

#endif // STRING_FORMAT_H_
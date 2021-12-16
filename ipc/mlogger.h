

#ifndef MLOGGER_H_
#define MLOGGER_H_

#include <string>

bool initDebugBuffer();
size_t available();
size_t consumed();
bool isEmpty();
bool isFull();
size_t dbgWrite(const std::string& s);
size_t dbgWrite(int i);
bool dbgPop(char* c, size_t& len);
std::string dbgPop();

#endif // MLOGGER_H_
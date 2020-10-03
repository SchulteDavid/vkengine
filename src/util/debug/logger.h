#ifndef LOGGER_H
#define LOGGER_H

#include <ostream>
#include <iostream>

#ifdef DEBUG_LOGGING
#define logger(x) (x << "\033[0;33m[" << __FILE__ << ":" << __PRETTY_FUNCTION__ << ":" << __LINE__ << "]:\033[0m ")
#define lout logger(std::cout)
#define errlogger(x) (x << "\033[1;31m ["<< __FILE__ << ":" << __PRETTY_FUNCTION__ << ":" << __LINE__ << "]:\033[0m ")
#define lerr errlogger(std::cerr)
#else
#define logger(x) x
#define lout std::cout
#define lerr std::cerr
#define errlogger(x) x
#endif // DEBUG_LOGGING

#endif // LOGGER_H

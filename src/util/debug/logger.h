#ifndef LOGGER_H
#define LOGGER_H

#include <ostream>
#include <iostream>

#ifdef DEBUG_LOGGING
#define logger(x) (x << "[" << __FILE__ << ":" << __PRETTY_FUNCTION__ << ":" << __LINE__ << "]: ")
#else
#define logger(x) x
#endif // DEBUG_LOGGING

#endif // LOGGER_H

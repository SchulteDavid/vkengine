#ifndef LOGGER_H
#define LOGGER_H

#include <ostream>
#include <iostream>

#define logger(x) (x << "[" << __FILE__ << ":" << __PRETTY_FUNCTION__ << ":" << __LINE__ << "]: ")

#endif // LOGGER_H

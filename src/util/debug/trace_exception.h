#ifndef TRACE_EXCEPTION_H
#define TRACE_EXCEPTION_H

#include <string>
#include <exception>
#include <vector>

namespace dbg {

class trace_exception : public std::exception {

    public:
        trace_exception(std::string msg, uint32_t max_frames=63);
        virtual ~trace_exception();

        const char * what () const throw ();

    protected:

    private:

        std::string msg;

};

}

#endif // TRACE_EXCEPTION_H

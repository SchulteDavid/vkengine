#include "trace_exception.h"

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>

dbg::trace_exception::trace_exception(std::string msg, uint32_t max_frames) {

    std::string userMsg;
    std::vector<std::string> stackData;

    userMsg = msg;

    void * addrList[max_frames+1];
    int frameCount = backtrace(addrList, max_frames+1);

    if (!frameCount) {
        return;
    }

    char ** symbolList = backtrace_symbols(addrList, frameCount);
    size_t funcNameSize = 256;
    char * funcName = (char *) malloc(funcNameSize * sizeof(char));

    for (int i = 1; i < frameCount; ++i) {

        char * begin_name = nullptr;
        char * begin_offset = nullptr;
        char * end_offset = nullptr;

        for (char *p = symbolList[i]; *p; ++p) {

            if (*p == '(') {

                begin_name = p;

            } else if (*p == '+') {

                begin_offset = p;

            } else if (*p == ')' && begin_offset) {

                end_offset = p;
                break;

            }
        }

        if (begin_name && begin_offset && end_offset && begin_offset < end_offset) {

            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            int status;
            char* ret = abi::__cxa_demangle(begin_name, funcName, &funcNameSize, &status);

            if (!ret) {

            }

            if (!status) {

                std::string entry("\t");
                entry.append(std::string(symbolList[i])).append(" : ");
                entry.append(std::string(funcName));

                stackData.push_back(entry);

            } else {

                std::string entry("\t");
                entry.append(std::string(symbolList[i])).append(" : ");
                entry.append(std::string(begin_name)).append("()");

                stackData.push_back(entry);

            }

        }

    }

    this->msg = "Exception: '";
    this->msg.append(userMsg).append("': \n");

    for (std::string s : stackData) {

        this->msg.append(s).append("\n");

    }

}

dbg::trace_exception::~trace_exception() {

}

const char * dbg::trace_exception::what() const throw () {

    return msg.c_str();

}

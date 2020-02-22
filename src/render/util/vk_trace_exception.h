#ifndef VK_TRACE_EXCEPTION_H
#define VK_TRACE_EXCEPTION_H

#include "util/debug/trace_exception.h"
#include <vulkan/vulkan.h>

namespace vkutil {

class vk_trace_exception : public dbg::trace_exception
{
    public:
        vk_trace_exception(std::string msg, VkResult r);
        virtual ~vk_trace_exception();

    protected:

    private:
};

}

#endif // VK_TRACE_EXCEPTION_H

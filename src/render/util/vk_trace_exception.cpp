#include "vk_trace_exception.h"

std::string getErrorCodeName(VkResult res) {

    switch (res) {

        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "Out of host memory";

        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "Out of device memory";

        case VK_ERROR_DEVICE_LOST:
            return "Device lost";

        default:
            return std::string("VkResult ").append(std::to_string((int) res));

    }

}

vkutil::vk_trace_exception::vk_trace_exception(std::string msg, VkResult r) : dbg::trace_exception(msg.append(" : ").append(getErrorCodeName(r))) {


}

vkutil::vk_trace_exception::~vk_trace_exception() {

}

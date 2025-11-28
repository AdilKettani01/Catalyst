/**
 * @file vulkan_utils.hpp
 * @brief Small Vulkan helpers.
 */

#pragma once

#include <vulkan/vulkan.h>

namespace manim::vulkan_utils {

inline VkCommandBufferBeginInfo one_time_begin_info() {
    VkCommandBufferBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    return info;
}

inline VkSubmitInfo submit_info(const VkCommandBuffer* cmd) {
    VkSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = cmd;
    return info;
}

}  // namespace manim::vulkan_utils

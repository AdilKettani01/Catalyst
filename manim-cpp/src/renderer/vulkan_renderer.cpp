// Minimal Vulkan renderer that drives the GPU3DRenderer when a device is available.
#include "manim/renderer/vulkan_renderer.hpp"

#include "manim/renderer/gpu_3d_renderer.hpp"
#include "manim/scene/scene.h"
#include "manim/scene/camera.hpp"
#include "manim/renderer/vulkan_utils.hpp"
#include "manim/core/gpu_profiler.hpp"
#include "manim/core/async_compute.hpp"
#include <spdlog/spdlog.h>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <limits>
#include <algorithm>
#include <string>
#include <chrono>

namespace manim {

VulkanRenderer::VulkanRenderer() {
    memory_pool_ = std::make_unique<MemoryPool>();
}

VulkanRenderer::~VulkanRenderer() {
    shutdown();
}

void VulkanRenderer::initialize(const RendererConfig& config) {
    if (initialized_) {
        return;
    }

    config_ = config;
    stats_ = {};
    clear_color_ = math::Vec4{0, 0, 0, 1};

    create_instance();
    windowed_ = (std::getenv("MANIM_WINDOWED") && std::string(std::getenv("MANIM_WINDOWED")) != "0");
    if (windowed_) {
        create_window_surface();
    }
    pick_physical_device();
    create_logical_device();
    create_command_pool_and_buffer();
    if (windowed_ && device_ != VK_NULL_HANDLE) {
        create_swapchain();
        create_sync_objects();
    }

    gpu_renderer_ = std::make_unique<GPU3DRenderer>();

    if (device_ != VK_NULL_HANDLE && physical_device_ != VK_NULL_HANDLE && command_buffer_ != VK_NULL_HANDLE) {
        try {
            memory_pool_->initialize(device_, physical_device_, instance_);
            memory_pool_->set_transfer_resources(command_pool_, graphics_queue_, graphics_queue_family_);
            RendererConfig gpu_config = config_;
            gpu_config.enable_deferred_rendering = true;
            gpu_renderer_->initialize(
                device_,
                physical_device_,
                graphics_queue_,
                graphics_queue_family_,
                *memory_pool_,
                windowed_ && swapchain_extent_.width > 0 ? swapchain_extent_.width : config_.width,
                windowed_ && swapchain_extent_.height > 0 ? swapchain_extent_.height : config_.height,
                config_.msaa_samples  // MSAA sample count from renderer config
            );
            gpu_ready_ = true;
            spdlog::info("VulkanRenderer initialized with GPU.");

            // Initialize performance optimization systems
            initializeGlobalGPUProfiler(device_, physical_device_);
            initializeGlobalAsyncCompute(device_, physical_device_, graphics_queue_family_, memory_pool_.get());
        } catch (const std::exception& e) {
            spdlog::warn("Failed to initialize GPU renderer path: {}", e.what());
            gpu_ready_ = false;
        }
    } else {
        spdlog::warn("Vulkan device/command buffer not ready; running in no-op GPU mode.");
        gpu_ready_ = false;
    }

    initialized_ = true;
}

void VulkanRenderer::shutdown() {
    if (!initialized_) {
        return;
    }

    // Shutdown performance systems first
    if (isGPUProfilerInitialized()) {
        getGlobalGPUProfiler().print_report();
        shutdownGlobalGPUProfiler();
    }
    shutdownGlobalAsyncCompute();

    gpu_ready_ = false;
    if (gpu_renderer_) {
        gpu_renderer_.reset();
    }

    if (memory_pool_) {
        memory_pool_->shutdown();
    }

    destroy_vulkan();
    initialized_ = false;
}

void VulkanRenderer::begin_frame() {
    begin_frame_timing();

    if (!gpu_ready_) {
        return;
    }

    if (windowed_ && swapchain_ != VK_NULL_HANDLE) {
        // Check if framebuffer was resized
        if (framebuffer_resized_) {
            framebuffer_resized_ = false;
            recreate_swapchain();
        }

        // Wait for previous frame's fence
        vkWaitForFences(device_, 1, &in_flight_fence_, VK_TRUE, UINT64_MAX);

        VkResult acquire = vkAcquireNextImageKHR(
            device_,
            swapchain_,
            std::numeric_limits<uint64_t>::max(),
            image_available_,
            VK_NULL_HANDLE,
            &acquired_image_index_
        );

        if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain();
            return;  // Skip this frame
        } else if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
            spdlog::error("Failed to acquire swapchain image");
            return;
        }

        vkResetFences(device_, 1, &in_flight_fence_);
    }

    // Reset and begin command buffer
    vkResetCommandBuffer(command_buffer_, 0);
    auto begin_info = vulkan_utils::one_time_begin_info();
    vkBeginCommandBuffer(command_buffer_, &begin_info);

    // Start GPU profiling for this frame
    if (isGPUProfilerInitialized()) {
        getGlobalGPUProfiler().begin_frame(command_buffer_);
    }
}

void VulkanRenderer::end_frame() {
    if (gpu_ready_) {
        if (windowed_ && swapchain_ != VK_NULL_HANDLE) {
            copy_to_swapchain();
        }

        // End GPU profiling for this frame
        if (isGPUProfilerInitialized()) {
            getGlobalGPUProfiler().end_frame(command_buffer_);
        }

        vkEndCommandBuffer(command_buffer_);

        if (windowed_ && swapchain_ != VK_NULL_HANDLE) {
            VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo submit{};
            submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit.waitSemaphoreCount = 1;
            submit.pWaitSemaphores = &image_available_;
            submit.pWaitDstStageMask = &wait_stage;
            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &command_buffer_;
            submit.signalSemaphoreCount = 1;
            submit.pSignalSemaphores = &render_finished_;

            vkQueueSubmit(graphics_queue_, 1, &submit, in_flight_fence_);

            VkPresentInfoKHR present{};
            present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present.waitSemaphoreCount = 1;
            present.pWaitSemaphores = &render_finished_;
            present.swapchainCount = 1;
            present.pSwapchains = &swapchain_;
            present.pImageIndices = &acquired_image_index_;

            VkResult result = vkQueuePresentKHR(graphics_queue_, &present);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized_) {
                framebuffer_resized_ = false;
                recreate_swapchain();
            } else if (result != VK_SUCCESS) {
                spdlog::error("Failed to present swapchain image");
            }
        } else {
            VkSubmitInfo submit = vulkan_utils::submit_info(&command_buffer_);
            vkQueueSubmit(graphics_queue_, 1, &submit, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphics_queue_);
        }

        // Collect profiler results and update stats
        if (isGPUProfilerInitialized()) {
            auto& profiler = getGlobalGPUProfiler();
            profiler.collect_results();
            stats_.gpu_time_ms = profiler.get_last_frame_gpu_time_ms();
        }

        // Poll async compute completions
        if (isAsyncComputeAvailable()) {
            getGlobalAsyncCompute().poll_completions();
        }
    }

    end_frame_timing();
}

void VulkanRenderer::render_scene(Scene& scene, Camera& camera) {
    if (!gpu_ready_ || !gpu_renderer_) {
        spdlog::warn("render_scene called without a ready GPU renderer; skipping.");
        return;
    }

    gpu_renderer_->render(command_buffer_, scene, camera);
    stats_.frame_number++;
}

void VulkanRenderer::render_mobject(Mobject& mobject) {
    if (!gpu_ready_ || !gpu_renderer_) {
        return;
    }

    // Reset and begin command buffer for standalone render
    vkResetCommandBuffer(command_buffer_, 0);
    auto begin_info = vulkan_utils::one_time_begin_info();
    vkBeginCommandBuffer(command_buffer_, &begin_info);

    // Create temporary scene with the mobject
    // Pass a non-owning shared_ptr to this renderer to avoid Scene creating its own
    auto self_renderer = std::shared_ptr<Renderer>(std::shared_ptr<Renderer>{}, this);
    Scene temp_scene(self_renderer);

    // Add mobject with non-owning shared_ptr
    auto mob_ptr = std::shared_ptr<Mobject>(std::shared_ptr<Mobject>{}, &mobject);
    temp_scene.add(mob_ptr);

    Camera camera;
    gpu_renderer_->render(command_buffer_, temp_scene, camera);

    // End and submit command buffer
    vkEndCommandBuffer(command_buffer_);

    VkSubmitInfo submit = vulkan_utils::submit_info(&command_buffer_);
    vkQueueSubmit(graphics_queue_, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue_);
}

void VulkanRenderer::clear(const math::Vec4& color) {
    clear_color_ = color;
}

void VulkanRenderer::resize(uint32_t width, uint32_t height) {
    config_.width = width;
    config_.height = height;

    if (gpu_ready_ && gpu_renderer_) {
        gpu_renderer_->resize(width, height);
    }
}

void VulkanRenderer::capture_frame(const std::string& output_path) {
    if (!gpu_ready_ || !gpu_renderer_) {
        spdlog::warn("capture_frame called without GPU ready.");
        return;
    }

    const auto& img = gpu_renderer_->get_final_image();
    if (img.get_image() == VK_NULL_HANDLE) {
        spdlog::warn("capture_frame: final image is null.");
        return;
    }

    VkDeviceSize image_size = static_cast<VkDeviceSize>(config_.width) *
                              static_cast<VkDeviceSize>(config_.height) * 4;

    // Allocate staging buffer
    GPUBuffer staging = memory_pool_->allocate_buffer(
        image_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryType::Staging,
        MemoryUsage::Streaming
    );

    // Record copy commands
    vkResetCommandBuffer(command_buffer_, 0);
    auto begin_info = vulkan_utils::one_time_begin_info();
    vkBeginCommandBuffer(command_buffer_, &begin_info);

    // Transition image to transfer src
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = img.get_image();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(
        command_buffer_,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    VkBufferImageCopy copy{};
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset = {0, 0, 0};
    copy.imageExtent = {config_.width, config_.height, 1};

    vkCmdCopyImageToBuffer(
        command_buffer_,
        img.get_image(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        staging.get_buffer(),
        1,
        &copy
    );

    // Transition back to GENERAL
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(
        command_buffer_,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkEndCommandBuffer(command_buffer_);

    VkSubmitInfo submit = vulkan_utils::submit_info(&command_buffer_);
    vkQueueSubmit(graphics_queue_, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue_);

    // Map and write PPM
    void* data = staging.map();
    if (!data) {
        spdlog::warn("capture_frame: failed to map staging buffer.");
        return;
    }

    std::string path = output_path.empty() ? std::string("capture.ppm") : output_path;
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        spdlog::warn("capture_frame: failed to open {} for writing.", path);
        staging.unmap();
        return;
    }

    out << "P6\n" << config_.width << " " << config_.height << "\n255\n";
    const uint8_t* rgba = static_cast<const uint8_t*>(data);
    for (uint32_t y = 0; y < config_.height; ++y) {
        for (uint32_t x = 0; x < config_.width; ++x) {
            size_t idx = (static_cast<size_t>(y) * config_.width + x) * 4;
            out.put(static_cast<char>(rgba[idx + 0]));
            out.put(static_cast<char>(rgba[idx + 1]));
            out.put(static_cast<char>(rgba[idx + 2]));
        }
    }
    staging.unmap();
    spdlog::info("Captured frame to {}", path);
}

bool VulkanRenderer::supports_feature(const std::string& feature) const {
    return (feature == "vulkan") || (feature == "gpu") || (feature == "raytracing" && config_.enable_ray_tracing);
}

void VulkanRenderer::create_instance() {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Manim C++";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Manim";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
        spdlog::warn("Failed to create Vulkan instance.");
        instance_ = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::framebuffer_resize_callback(GLFWwindow* window, int /*width*/, int /*height*/) {
    auto* renderer = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    if (renderer) {
        renderer->framebuffer_resized_ = true;
    }
}

void VulkanRenderer::create_window_surface() {
    if (!glfwInit()) {
        spdlog::warn("GLFW init failed; falling back to headless.");
        windowed_ = false;
        return;
    }
    if (!glfwVulkanSupported()) {
        spdlog::warn("GLFW reports Vulkan not supported; falling back to headless.");
        windowed_ = false;
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(static_cast<int>(config_.width), static_cast<int>(config_.height), "Manim C++", nullptr, nullptr);
    if (!window_) {
        spdlog::warn("Failed to create GLFW window; falling back to headless.");
        windowed_ = false;
        return;
    }

    // Set up user pointer and resize callback
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);

    if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
        spdlog::warn("Failed to create Vulkan surface; falling back to headless.");
        windowed_ = false;
        return;
    }
}

void VulkanRenderer::pick_physical_device() {
    if (instance_ == VK_NULL_HANDLE) {
        return;
    }

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    if (device_count == 0) {
        spdlog::warn("No Vulkan-capable GPUs found.");
        return;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

    for (auto device : devices) {
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> props(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, props.data());

        for (uint32_t i = 0; i < queue_family_count; ++i) {
            bool has_graphics = props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
            bool has_present = true;
            if (windowed_ && surface_ != VK_NULL_HANDLE) {
                VkBool32 support = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &support);
                has_present = support == VK_TRUE;
            }
            if (has_graphics && has_present) {
                physical_device_ = device;
                graphics_queue_family_ = i;
                return;
            }
        }
    }

    spdlog::warn("No suitable graphics queue found on available GPUs.");
}

void VulkanRenderer::create_logical_device() {
    if (physical_device_ == VK_NULL_HANDLE) {
        return;
    }

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = graphics_queue_family_;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    // Enable buffer device address feature (required for VMA with SHADER_DEVICE_ADDRESS)
    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features{};
    buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    buffer_device_address_features.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &buffer_device_address_features;

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = &features2;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_info;

    if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS) {
        spdlog::warn("Failed to create Vulkan device.");
        device_ = VK_NULL_HANDLE;
        return;
    }

    vkGetDeviceQueue(device_, graphics_queue_family_, 0, &graphics_queue_);
}

void VulkanRenderer::create_command_pool_and_buffer() {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = graphics_queue_family_;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
        spdlog::warn("Failed to create Vulkan command pool.");
        command_pool_ = VK_NULL_HANDLE;
        return;
    }

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer_) != VK_SUCCESS) {
        spdlog::warn("Failed to allocate Vulkan command buffer.");
        command_buffer_ = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::copy_to_swapchain() {
    if (!gpu_renderer_ || swapchain_ == VK_NULL_HANDLE || acquired_image_index_ >= swapchain_images_.size()) {
        return;
    }

    const GPUImage& src_img = gpu_renderer_->get_final_image();
    VkImage src = src_img.get_image();
    VkImage dst = swapchain_images_[acquired_image_index_];
    if (src == VK_NULL_HANDLE || dst == VK_NULL_HANDLE) {
        return;
    }

    VkImageMemoryBarrier barriers[2]{};
    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].image = src;
    barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[0].subresourceRange.baseMipLevel = 0;
    barriers[0].subresourceRange.levelCount = 1;
    barriers[0].subresourceRange.baseArrayLayer = 0;
    barriers[0].subresourceRange.layerCount = 1;
    barriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].image = dst;
    barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[1].subresourceRange.baseMipLevel = 0;
    barriers[1].subresourceRange.levelCount = 1;
    barriers[1].subresourceRange.baseArrayLayer = 0;
    barriers[1].subresourceRange.layerCount = 1;
    barriers[1].srcAccessMask = 0;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        command_buffer_,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        2,
        barriers
    );

    VkImageCopy region{};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.extent = {swapchain_extent_.width, swapchain_extent_.height, 1};

    vkCmdCopyImage(
        command_buffer_,
        src,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    VkImageMemoryBarrier present_barrier{};
    present_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    present_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    present_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    present_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    present_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    present_barrier.image = dst;
    present_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    present_barrier.subresourceRange.baseMipLevel = 0;
    present_barrier.subresourceRange.levelCount = 1;
    present_barrier.subresourceRange.baseArrayLayer = 0;
    present_barrier.subresourceRange.layerCount = 1;
    present_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    present_barrier.dstAccessMask = VK_ACCESS_NONE;

    vkCmdPipelineBarrier(
        command_buffer_,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1,
        &present_barrier
    );
}

void VulkanRenderer::create_swapchain() {
    if (!windowed_ || surface_ == VK_NULL_HANDLE || physical_device_ == VK_NULL_HANDLE || device_ == VK_NULL_HANDLE) {
        return;
    }

    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &caps);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, formats.data());

    VkSurfaceFormatKHR chosen_format = formats.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR} : formats[0];
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM) {
            chosen_format = f;
            break;
        }
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &present_mode_count, nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &present_mode_count, present_modes.data());
    for (auto pm : present_modes) {
        if (pm == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = pm;
            break;
        }
    }

    VkExtent2D extent{};
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = caps.currentExtent;
    } else {
        extent.width = std::clamp(config_.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(config_.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create{};
    create.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create.surface = surface_;
    create.minImageCount = image_count;
    create.imageFormat = chosen_format.format;
    create.imageColorSpace = chosen_format.colorSpace;
    create.imageExtent = extent;
    create.imageArrayLayers = 1;
    create.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create.preTransform = caps.currentTransform;
    create.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create.presentMode = present_mode;
    create.clipped = VK_TRUE;
    create.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device_, &create, nullptr, &swapchain_) != VK_SUCCESS) {
        spdlog::warn("Failed to create swapchain; continuing headless.");
        windowed_ = false;
        return;
    }

    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());

    swapchain_format_ = chosen_format.format;
    swapchain_extent_ = extent;

    swapchain_image_views_.clear();
    swapchain_image_views_.reserve(swapchain_images_.size());
    for (auto image : swapchain_images_) {
        VkImageViewCreateInfo view{};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = swapchain_format_;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.baseMipLevel = 0;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount = 1;

        VkImageView iv = VK_NULL_HANDLE;
        if (vkCreateImageView(device_, &view, nullptr, &iv) != VK_SUCCESS) {
            swapchain_image_views_.push_back(VK_NULL_HANDLE);
        } else {
            swapchain_image_views_.push_back(iv);
        }
    }
}

void VulkanRenderer::destroy_swapchain() {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }
    if (render_finished_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device_, render_finished_, nullptr);
        render_finished_ = VK_NULL_HANDLE;
    }
    if (image_available_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device_, image_available_, nullptr);
        image_available_ = VK_NULL_HANDLE;
    }
    if (in_flight_fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device_, in_flight_fence_, nullptr);
        in_flight_fence_ = VK_NULL_HANDLE;
    }
    for (auto iv : swapchain_image_views_) {
        if (iv != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, iv, nullptr);
        }
    }
    swapchain_image_views_.clear();
    swapchain_images_.clear();
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::create_sync_objects() {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }
    VkSemaphoreCreateInfo sem_info{};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device_, &sem_info, nullptr, &image_available_) != VK_SUCCESS ||
        vkCreateSemaphore(device_, &sem_info, nullptr, &render_finished_) != VK_SUCCESS ||
        vkCreateFence(device_, &fence_info, nullptr, &in_flight_fence_) != VK_SUCCESS) {
        spdlog::warn("Failed to create sync objects; presenting disabled.");
        windowed_ = false;
    }
}

void VulkanRenderer::destroy_vulkan() {
    destroy_swapchain();
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }

    if (device_ != VK_NULL_HANDLE) {
        if (command_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, command_pool_, nullptr);
            command_pool_ = VK_NULL_HANDLE;
            command_buffer_ = VK_NULL_HANDLE;
        }
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::recreate_swapchain() {
    // Handle minimized windows
    int width = 0, height = 0;
    if (window_) {
        glfwGetFramebufferSize(window_, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window_, &width, &height);
            glfwWaitEvents();
        }
    }

    vkDeviceWaitIdle(device_);

    destroy_swapchain();
    create_swapchain();
    create_sync_objects();

    // Update renderer config and GPU renderer
    if (swapchain_extent_.width > 0 && swapchain_extent_.height > 0) {
        config_.width = swapchain_extent_.width;
        config_.height = swapchain_extent_.height;

        if (gpu_renderer_) {
            gpu_renderer_->resize(swapchain_extent_.width, swapchain_extent_.height);
        }
    }

    spdlog::info("Swapchain recreated: {}x{}", swapchain_extent_.width, swapchain_extent_.height);
}

void VulkanRenderer::run_render_loop(std::function<void(float dt)> render_callback) {
    if (!windowed_ || !window_) {
        spdlog::warn("render_loop called but not in windowed mode");
        return;
    }

    auto last_time = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        auto current_time = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;

        begin_frame();
        if (render_callback) {
            render_callback(dt);
        }
        end_frame();
    }

    // Wait for device to finish before cleanup
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
}

void VulkanRenderer::poll_events() {
    glfwPollEvents();
}

bool VulkanRenderer::should_close() const {
    return window_ && glfwWindowShouldClose(window_);
}

void VulkanRenderer::handle_resize(int width, int height) {
    if (width > 0 && height > 0) {
        framebuffer_resized_ = true;
        config_.width = static_cast<uint32_t>(width);
        config_.height = static_cast<uint32_t>(height);
    }
}

}  // namespace manim

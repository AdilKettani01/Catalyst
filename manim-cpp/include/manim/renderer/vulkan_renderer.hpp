/**
 * @file vulkan_renderer.hpp
 * @brief Vulkan rendering backend implementation
 */

#pragma once

#include "renderer.hpp"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <functional>

namespace manim {

class GPU3DRenderer;

class VulkanRenderer : public Renderer {
public:
    VulkanRenderer();
    ~VulkanRenderer() override;

    void initialize() { initialize(RendererConfig{}); }
    void initialize(const RendererConfig& config) override;
    void shutdown() override;
    void begin_frame() override;
    void end_frame() override;
    void render_scene(Scene& scene, Camera& camera) override;
    void render_mobject(Mobject& mobject) override;
    void clear(const math::Vec4& color = math::Vec4{0, 0, 0, 1}) override;
    void resize(uint32_t width, uint32_t height) override;
    const FrameStats& get_stats() const override { return stats_; }
    void capture_frame(const std::string& output_path) override;
    RendererType get_type() const override { return RendererType::Vulkan; }
    bool supports_feature(const std::string& feature) const override;
    MemoryPool& get_memory_pool() override { return *memory_pool_; }
    void set_render_mode(RenderMode mode) override { render_mode_ = mode; }

    /// Indicates whether a Vulkan device and command buffer are ready.
    bool is_gpu_ready() const { return gpu_ready_; }

    /// Run interactive render loop (blocking, handles events)
    void run_render_loop(std::function<void(float dt)> render_callback);

    /// Poll events without blocking
    void poll_events();

    /// Check if window should close
    bool should_close() const;

    /// Handle framebuffer resize
    void handle_resize(int width, int height);

    /// Get GLFW window (for external event handling)
    GLFWwindow* get_window() const { return window_; }

private:
    // Vulkan resources
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    uint32_t graphics_queue_family_ = 0;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat swapchain_format_ = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchain_extent_{};
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    VkSemaphore image_available_ = VK_NULL_HANDLE;
    VkSemaphore render_finished_ = VK_NULL_HANDLE;
    VkFence in_flight_fence_ = VK_NULL_HANDLE;
    uint32_t acquired_image_index_ = 0;
    bool windowed_ = false;
    bool framebuffer_resized_ = false;
    GLFWwindow* window_ = nullptr;

    std::unique_ptr<GPU3DRenderer> gpu_renderer_;
    bool gpu_ready_ = false;
    bool initialized_ = false;
    math::Vec4 clear_color_{0, 0, 0, 1};

    // Helpers
    void create_instance();
    void create_window_surface();
    void pick_physical_device();
    void create_logical_device();
    void create_command_pool_and_buffer();
    void create_swapchain();
    void destroy_swapchain();
    void recreate_swapchain();
    void create_sync_objects();
    void copy_to_swapchain();
    void destroy_vulkan();
    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
};

} // namespace manim

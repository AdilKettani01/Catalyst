// Advanced GPU 3D renderer implementation (stubbed for compilation)
#include "manim/renderer/gpu_3d_renderer.hpp"
#include "manim/mobject/vmobject.hpp"
#include "manim/mobject/text/text.hpp"
#include "manim/scene/scene.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace manim {

// Helper to convert uint32_t sample count to VkSampleCountFlagBits
static VkSampleCountFlagBits to_sample_count(uint32_t samples) {
    switch (samples) {
        case 1:  return VK_SAMPLE_COUNT_1_BIT;
        case 2:  return VK_SAMPLE_COUNT_2_BIT;
        case 4:  return VK_SAMPLE_COUNT_4_BIT;
        case 8:  return VK_SAMPLE_COUNT_8_BIT;
        case 16: return VK_SAMPLE_COUNT_16_BIT;
        default: return VK_SAMPLE_COUNT_4_BIT;  // Default to 4x MSAA
    }
}

GPU3DRenderer::GPU3DRenderer() {
    spdlog::info("Initializing GPU 3D renderer with deferred pipeline");
}

GPU3DRenderer::~GPU3DRenderer() {
    shutdown();
}

void GPU3DRenderer::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkQueue graphics_queue,
    uint32_t graphics_queue_family,
    MemoryPool& memory_pool,
    uint32_t width,
    uint32_t height,
    uint32_t msaa_samples
) {
    device_ = device;
    physical_device_ = physical_device;
    graphics_queue_ = graphics_queue;
    graphics_queue_family_ = graphics_queue_family;
    memory_pool_ = &memory_pool;
    width_ = width;
    height_ = height;
    msaa_samples_ = to_sample_count(msaa_samples);
    const char* swap_env = std::getenv("MANIM_ENABLE_SWAPCHAIN");
    bool env_swap_enabled = swap_env && swap_env[0] != '0';
    use_swapchain_ = config_.enable_swapchain || env_swap_enabled;
    swapchain_image_count_ = std::max<uint32_t>(1, config_.swapchain_image_count);

    config_.width = width;
    config_.height = height;
    config_.shadow_config = shadow_config_;
    config_.gi_config = gi_config_;
    config_.postprocess_config = post_config_;

    create_gbuffer();
    create_shadow_maps();
    create_voxel_grid();
    create_render_targets();
    create_render_pass();
    create_framebuffers();
    create_pipelines();

    // Initialize text renderer with matching MSAA sample count
    if (render_pass_ != VK_NULL_HANDLE && memory_pool_) {
        text_renderer_initialized_ = text_renderer_.initialize(
            device_,
            physical_device_,
            render_pass_,
            *memory_pool_,
            0,  // subpass 0
            msaa_samples_  // Pass MSAA sample count for pipeline compatibility
        );
        if (text_renderer_initialized_) {
            spdlog::info("TextRenderer initialized for GPU3DRenderer (MSAA: {}x)",
                         static_cast<int>(msaa_samples_));
        } else {
            spdlog::warn("TextRenderer initialization failed - text rendering disabled");
        }
    }

    spdlog::info("GPU 3D renderer initialized: {}x{}, MSAA: {}x", width, height, msaa_samples);
}

void GPU3DRenderer::shutdown() {
    destroy_resources();
    spdlog::info("GPU 3D renderer shut down");
}

void GPU3DRenderer::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    config_.width = width;
    config_.height = height;

    destroy_resources();
    create_gbuffer();
    create_render_targets();
    create_render_pass();
    create_framebuffers();
    create_pipelines();

    spdlog::info("Resized GPU 3D renderer: {}x{}", width, height);
}

void GPU3DRenderer::render(
    VkCommandBuffer cmd,
    Scene& scene,
    Camera& camera
) {
    if (device_ == VK_NULL_HANDLE || graphics_queue_ == VK_NULL_HANDLE) {
        spdlog::warn("GPU3DRenderer render called without a valid device/queue.");
        return;
    }
    if (!memory_pool_) {
        spdlog::warn("GPU3DRenderer render missing memory pool.");
        return;
    }

    frame_vertex_buffers_.clear();
    frame_index_buffers_.clear();
    batched_draws_.clear();

    uint32_t frame_index = (use_swapchain_ && !swapchain_images_.empty()) ? swapchain_index_ : 0;
    const GPUImage& color_image = (use_swapchain_ && !swapchain_images_.empty())
        ? swapchain_images_[frame_index]
        : final_image_;

    // Minimal GPU path: clear the final image using Vulkan commands.
    VkImage image = color_image.get_image();
    VkFramebuffer target_framebuffer = framebuffer_;
    if (use_swapchain_ && frame_index < swapchain_framebuffers_.size() && swapchain_framebuffers_[frame_index] != VK_NULL_HANDLE) {
        target_framebuffer = swapchain_framebuffers_[frame_index];
    }

    const bool use_msaa = (msaa_samples_ != VK_SAMPLE_COUNT_1_BIT);

    // Check required resources based on MSAA mode
    if (use_msaa) {
        if (image == VK_NULL_HANDLE || render_pass_ == VK_NULL_HANDLE ||
            target_framebuffer == VK_NULL_HANDLE || triangle_pipeline_ == VK_NULL_HANDLE ||
            msaa_color_image_.get_image() == VK_NULL_HANDLE ||
            msaa_depth_image_.get_image() == VK_NULL_HANDLE) {
            spdlog::warn("GPU3DRenderer render missing MSAA resources for draw.");
            return;
        }
    } else {
        if (image == VK_NULL_HANDLE || render_pass_ == VK_NULL_HANDLE ||
            target_framebuffer == VK_NULL_HANDLE || triangle_pipeline_ == VK_NULL_HANDLE ||
            depth_image_.get_image() == VK_NULL_HANDLE) {
            spdlog::warn("GPU3DRenderer render missing resources for draw (image={}, rp={}, fb={}, pipe={}, depth={}).",
                (void*)image, (void*)render_pass_, (void*)target_framebuffer,
                (void*)triangle_pipeline_, (void*)depth_image_.get_image());
            return;
        }
    }

    // Set up image barriers for the render pass
    if (use_msaa) {
        // MSAA: transition MSAA color, MSAA depth, and resolve target
        VkImageMemoryBarrier barriers[3]{};

        // MSAA color image
        barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].image = msaa_color_image_.get_image();
        barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barriers[0].subresourceRange.baseMipLevel = 0;
        barriers[0].subresourceRange.levelCount = 1;
        barriers[0].subresourceRange.baseArrayLayer = 0;
        barriers[0].subresourceRange.layerCount = 1;
        barriers[0].srcAccessMask = 0;
        barriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // MSAA depth image
        barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[1].image = msaa_depth_image_.get_image();
        barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barriers[1].subresourceRange.baseMipLevel = 0;
        barriers[1].subresourceRange.levelCount = 1;
        barriers[1].subresourceRange.baseArrayLayer = 0;
        barriers[1].subresourceRange.layerCount = 1;
        barriers[1].srcAccessMask = 0;
        barriers[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // Resolve target (final/swapchain image)
        barriers[2].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[2].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[2].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[2].image = image;
        barriers[2].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barriers[2].subresourceRange.baseMipLevel = 0;
        barriers[2].subresourceRange.levelCount = 1;
        barriers[2].subresourceRange.baseArrayLayer = 0;
        barriers[2].subresourceRange.layerCount = 1;
        barriers[2].srcAccessMask = 0;
        barriers[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            3, barriers
        );
    } else {
        // Non-MSAA: transition color and depth images
        VkImageMemoryBarrier barriers[2]{};
        barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].image = image;
        barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barriers[0].subresourceRange.baseMipLevel = 0;
        barriers[0].subresourceRange.levelCount = 1;
        barriers[0].subresourceRange.baseArrayLayer = 0;
        barriers[0].subresourceRange.layerCount = 1;
        barriers[0].srcAccessMask = 0;
        barriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[1].image = depth_image_.get_image();
        barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barriers[1].subresourceRange.baseMipLevel = 0;
        barriers[1].subresourceRange.levelCount = 1;
        barriers[1].subresourceRange.baseArrayLayer = 0;
        barriers[1].subresourceRange.layerCount = 1;
        barriers[1].srcAccessMask = 0;
        barriers[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            2, barriers
        );
    }

    // Clear values: for MSAA we need 3 (msaa_color, msaa_depth, resolve)
    VkClearValue clear[3]{};
    clear[0].color = {{0.07f, 0.07f, 0.09f, 1.0f}};  // Color (MSAA or single-sampled)
    clear[1].depthStencil = {1.0f, 0};               // Depth
    clear[2].color = {{0.0f, 0.0f, 0.0f, 0.0f}};     // Resolve target (only used in MSAA)

    VkRenderPassBeginInfo rp_begin{};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.renderPass = render_pass_;
    rp_begin.framebuffer = target_framebuffer;
    rp_begin.renderArea.offset = {0, 0};
    rp_begin.renderArea.extent = {width_, height_};
    rp_begin.clearValueCount = use_msaa ? 3 : 2;  // MSAA needs 3 clear values
    rp_begin.pClearValues = clear;

    vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_pipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width_);
    viewport.height = static_cast<float>(height_);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {width_, height_};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Simple orthographic projection covering a fixed world area
    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    float world_height = 8.0f;  // Increased to match BasicRenderer frustum
    float world_width = world_height * aspect;

    // Compute pixels-per-unit for stroke width conversion (pixel -> world units)
    float ppu = (world_height > 0.0f) ? (static_cast<float>(height_) / world_height) : 1.0f;

    math::Mat4 proj = glm::ortho(
        -world_width * 0.5f, world_width * 0.5f,
        -world_height * 0.5f, world_height * 0.5f,
        -10.0f, 10.0f
    );

    struct PushConstants {
        math::Mat4 mvp;
        math::Vec4 color;
    };

    // Build batched geometry for all mobjects (including submobjects)
    const auto& root_mobjects = scene.get_mobjects();
    std::vector<std::shared_ptr<Mobject>> all_mobjects;
    all_mobjects.reserve(root_mobjects.size());
    for (const auto& root : root_mobjects) {
        if (!root) continue;
        auto family = root->get_family();
        all_mobjects.insert(all_mobjects.end(), family.begin(), family.end());
    }

    std::vector<math::Vec3> fill_vertices;
    std::vector<uint32_t> fill_indices;
    std::vector<math::Vec3> stroke_vertices;
    std::vector<uint32_t> stroke_indices;
    std::vector<math::Vec3> temp_vertices;
    std::vector<uint32_t> temp_indices;
    const uint32_t segments = std::max<uint32_t>(tessellation_segments_per_curve_, 8u);

    // Collect Text objects for separate rendering
    std::vector<std::pair<std::shared_ptr<Text>, math::Mat4>> text_objects;

    for (const auto& mobj : all_mobjects) {
        if (!mobj) continue;

        // Check for Text objects first (before VMobject since Text inherits from VMobject)
        auto text_obj = std::dynamic_pointer_cast<Text>(mobj);
        if (text_obj) {
            math::Mat4 world = mobj->get_world_transform();
            text_objects.push_back({text_obj, world});
            spdlog::debug("GPU3DRenderer: Found Text object '{}' with opacity {}",
                          text_obj->get_text(), text_obj->get_opacity());
            continue;  // Don't process as VMobject
        }

        auto vmobj = std::dynamic_pointer_cast<VMobject>(mobj);
        math::Mat4 world = mobj->get_world_transform();

        if (vmobj) {
            // Fill
            vmobj->build_fill_geometry(world, segments, temp_vertices, temp_indices);
            if (!temp_indices.empty() && temp_vertices.size() >= 3) {
                uint32_t vertex_offset = static_cast<uint32_t>(fill_vertices.size());
                uint32_t first_index = static_cast<uint32_t>(fill_indices.size());
                fill_vertices.reserve(fill_vertices.size() + temp_vertices.size());
                fill_indices.reserve(fill_indices.size() + temp_indices.size());
                fill_vertices.insert(fill_vertices.end(), temp_vertices.begin(), temp_vertices.end());
                fill_indices.insert(fill_indices.end(), temp_indices.begin(), temp_indices.end());

                math::Vec4 color = vmobj->get_fill_color();
                color.w *= vmobj->get_fill_opacity();

                batched_draws_.push_back(
                    {DrawBufferSet::FillBatch,
                     first_index,
                     static_cast<uint32_t>(temp_indices.size()),
                     static_cast<int32_t>(vertex_offset),
                     color,
                     VK_NULL_HANDLE,
                     VK_NULL_HANDLE}
                );
            }

            // Stroke - convert pixel-based stroke width to world units
            float stroke_world = vmobj->get_stroke_width() / std::max(ppu, 1e-4f);
            vmobj->build_stroke_geometry(world, segments, temp_vertices, temp_indices, stroke_world);
            if (!temp_indices.empty() && temp_vertices.size() >= 4) {
                uint32_t vertex_offset = static_cast<uint32_t>(stroke_vertices.size());
                uint32_t first_index = static_cast<uint32_t>(stroke_indices.size());
                stroke_vertices.reserve(stroke_vertices.size() + temp_vertices.size());
                stroke_indices.reserve(stroke_indices.size() + temp_indices.size());
                stroke_vertices.insert(stroke_vertices.end(), temp_vertices.begin(), temp_vertices.end());
                stroke_indices.insert(stroke_indices.end(), temp_indices.begin(), temp_indices.end());

                math::Vec4 color = vmobj->get_stroke_color();
                color.w *= vmobj->get_stroke_opacity();

                batched_draws_.push_back(
                    {DrawBufferSet::StrokeBatch,
                     first_index,
                     static_cast<uint32_t>(temp_indices.size()),
                     static_cast<int32_t>(vertex_offset),
                     color,
                     VK_NULL_HANDLE,
                     VK_NULL_HANDLE}
                );
            }

            continue;
        }

        // Non-VMobject fallback: draw anchors as fan
        auto pts = mobj->get_points();
        if (pts.size() < 3) continue;

        // Apply world transform on CPU for now
        std::vector<math::Vec3> transformed;
        transformed.reserve(pts.size());
        for (const auto& p : pts) {
            math::Vec4 hp = world * math::Vec4(p, 1.0f);
            transformed.push_back(math::Vec3(hp));
        }

        if (transformed.size() < 3) continue;
        std::vector<uint32_t> indices;
        indices.reserve((transformed.size() - 2) * 3);
        for (uint32_t i = 1; i + 1 < transformed.size(); ++i) {
            indices.push_back(0);
            indices.push_back(i);
            indices.push_back(i + 1);
        }

        VkDeviceSize vb_size = transformed.size() * sizeof(math::Vec3);
        GPUBuffer vbuf = memory_pool_->allocate_buffer(
            vb_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );

        void* mapped = vbuf.map();
        if (mapped) {
            std::memcpy(mapped, transformed.data(), static_cast<size_t>(vb_size));
            vbuf.unmap();
        }

        VkDeviceSize ib_size = indices.size() * sizeof(uint32_t);
        GPUBuffer ibuf = memory_pool_->allocate_buffer(
            ib_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            MemoryType::HostVisible,
            MemoryUsage::Dynamic
        );

        void* imapped = ibuf.map();
        if (imapped) {
            std::memcpy(imapped, indices.data(), static_cast<size_t>(ib_size));
            ibuf.unmap();
        }

        frame_vertex_buffers_.push_back(std::move(vbuf));
        frame_index_buffers_.push_back(std::move(ibuf));

        BatchedDraw draw{};
        draw.buffer_set = DrawBufferSet::Custom;
        draw.first_index = 0;
        draw.index_count = static_cast<uint32_t>(indices.size());
        draw.vertex_offset = 0;
        draw.color = mobj->get_color();
        draw.custom_vertex = frame_vertex_buffers_.back().get_buffer();
        draw.custom_index = frame_index_buffers_.back().get_buffer();
        batched_draws_.push_back(draw);
    }

    // Debug logging for geometry generation
    spdlog::debug("GPU3DRenderer: {} mobjects, fill_verts={}, fill_idx={}, stroke_verts={}, stroke_idx={}, batched_draws={}",
                  all_mobjects.size(), fill_vertices.size(), fill_indices.size(),
                  stroke_vertices.size(), stroke_indices.size(), batched_draws_.size());

    upload_batched_buffers(
        fill_vertices,
        fill_indices,
        batched_fill_vertex_buffer_,
        batched_fill_index_buffer_,
        batched_fill_vertex_capacity_,
        batched_fill_index_capacity_
    );
    upload_batched_buffers(
        stroke_vertices,
        stroke_indices,
        batched_stroke_vertex_buffer_,
        batched_stroke_index_buffer_,
        batched_stroke_vertex_capacity_,
        batched_stroke_index_capacity_
    );

    DrawBufferSet bound_set = DrawBufferSet::Custom;
    for (const auto& draw : batched_draws_) {
        if (draw.index_count == 0) {
            continue;
        }

        switch (draw.buffer_set) {
            case DrawBufferSet::FillBatch: {
                VkBuffer vb = batched_fill_vertex_buffer_.get_buffer();
                VkBuffer ib = batched_fill_index_buffer_.get_buffer();
                if (vb == VK_NULL_HANDLE || ib == VK_NULL_HANDLE) {
                    continue;
                }
                if (bound_set != DrawBufferSet::FillBatch) {
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, offsets);
                    vkCmdBindIndexBuffer(cmd, ib, 0, VK_INDEX_TYPE_UINT32);
                    bound_set = DrawBufferSet::FillBatch;
                }
                break;
            }
            case DrawBufferSet::StrokeBatch: {
                VkBuffer vb = batched_stroke_vertex_buffer_.get_buffer();
                VkBuffer ib = batched_stroke_index_buffer_.get_buffer();
                if (vb == VK_NULL_HANDLE || ib == VK_NULL_HANDLE) {
                    continue;
                }
                if (bound_set != DrawBufferSet::StrokeBatch) {
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, offsets);
                    vkCmdBindIndexBuffer(cmd, ib, 0, VK_INDEX_TYPE_UINT32);
                    bound_set = DrawBufferSet::StrokeBatch;
                }
                break;
            }
            case DrawBufferSet::Custom: {
                if (draw.custom_vertex == VK_NULL_HANDLE || draw.custom_index == VK_NULL_HANDLE) {
                    continue;
                }
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, &draw.custom_vertex, offsets);
                vkCmdBindIndexBuffer(cmd, draw.custom_index, 0, VK_INDEX_TYPE_UINT32);
                bound_set = DrawBufferSet::Custom;
                break;
            }
        }

        PushConstants pc{};
        pc.mvp = proj;
        pc.color = draw.color;
        vkCmdPushConstants(cmd, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

        vkCmdDrawIndexed(cmd, draw.index_count, 1, draw.first_index, draw.vertex_offset, 0);
    }

    // Render text objects using TextRenderer
    if (text_renderer_initialized_ && !text_objects.empty()) {
        spdlog::debug("GPU3DRenderer: Rendering {} Text objects via TextRenderer", text_objects.size());

        // Set up projection matrix for text
        text_renderer_.set_matrices(proj, math::Mat4(1.0f));

        // Begin text batch
        text_renderer_.begin_batch(cmd);

        // Render each Text object
        for (auto& [text_ptr, model_matrix] : text_objects) {
            if (text_ptr) {
                text_renderer_.render_text(cmd, *text_ptr, model_matrix);
            }
        }

        // End text batch
        text_renderer_.end_batch(cmd);
    }

    swapchain_present_index_ = frame_index;
    vkCmdEndRenderPass(cmd);

    // Transition to GENERAL for potential sampling/reading
    VkImageMemoryBarrier end_barrier{};
    end_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    end_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    end_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    end_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    end_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    end_barrier.image = image;
    end_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    end_barrier.subresourceRange.baseMipLevel = 0;
    end_barrier.subresourceRange.levelCount = 1;
    end_barrier.subresourceRange.baseArrayLayer = 0;
    end_barrier.subresourceRange.layerCount = 1;
    end_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    end_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &end_barrier
    );
}

void GPU3DRenderer::advance_swapchain() {
    if (use_swapchain_ && !swapchain_images_.empty()) {
        swapchain_index_ = (swapchain_index_ + 1) % swapchain_images_.size();
    }
}

void GPU3DRenderer::render_geometry_pass(
    VkCommandBuffer /*cmd*/,
    Scene& /*scene*/,
    Camera& /*camera*/
) {
    spdlog::debug("Rendering geometry pass");
}

void GPU3DRenderer::render_lighting_pass(VkCommandBuffer /*cmd*/) {
    spdlog::debug("Rendering lighting pass");
}

void GPU3DRenderer::render_transparency_pass(
    VkCommandBuffer /*cmd*/,
    Scene& /*scene*/,
    Camera& /*camera*/
) {
    spdlog::debug("Rendering transparency pass");
}

void GPU3DRenderer::render_cascaded_shadow_maps(
    VkCommandBuffer /*cmd*/,
    Scene& /*scene*/,
    const math::Vec3& /*light_dir*/
) {
    spdlog::debug("Rendering cascaded shadow maps");
}

void GPU3DRenderer::render_ray_traced_shadows(
    VkCommandBuffer /*cmd*/,
    VkAccelerationStructureKHR /*tlas*/
) {
    spdlog::debug("Rendering ray-traced shadows");
}

void GPU3DRenderer::voxelize_scene(
    VkCommandBuffer /*cmd*/,
    Scene& /*scene*/
) {
    spdlog::debug("Voxelizing scene for VXGI");
}

void GPU3DRenderer::compute_voxel_gi(VkCommandBuffer /*cmd*/) {
    spdlog::debug("Computing VXGI");
}

void GPU3DRenderer::render_screen_space_reflections(VkCommandBuffer /*cmd*/) {
    spdlog::debug("Rendering screen-space reflections");
}

void GPU3DRenderer::render_ray_traced_gi(
    VkCommandBuffer /*cmd*/,
    VkAccelerationStructureKHR /*tlas*/
) {
    spdlog::debug("Rendering ray-traced GI");
}

void GPU3DRenderer::apply_taa(VkCommandBuffer /*cmd*/) {
    spdlog::debug("Applying TAA");
}

void GPU3DRenderer::apply_motion_blur(VkCommandBuffer /*cmd*/) {
    spdlog::debug("Applying motion blur");
}

void GPU3DRenderer::apply_dof(VkCommandBuffer /*cmd*/) {
    spdlog::debug("Applying depth of field");
}

void GPU3DRenderer::apply_bloom(VkCommandBuffer /*cmd*/) {
    spdlog::debug("Applying bloom");
}

void GPU3DRenderer::apply_tone_mapping(VkCommandBuffer /*cmd*/) {
    spdlog::debug("Applying tone mapping");
}

void GPU3DRenderer::post_process(VkCommandBuffer cmd) {
    if (post_config_.enable_taa) {
        apply_taa(cmd);
    }
    if (post_config_.enable_motion_blur) {
        apply_motion_blur(cmd);
    }
    if (post_config_.enable_dof) {
        apply_dof(cmd);
    }
    if (post_config_.enable_bloom) {
        apply_bloom(cmd);
    }
    apply_tone_mapping(cmd);
}

void GPU3DRenderer::create_gbuffer() {
    if (!memory_pool_) {
        return;
    }

    const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    gbuffer_.position = memory_pool_->allocate_image(
        width_,
        height_,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        usage
    );
    gbuffer_.normal = memory_pool_->allocate_image(
        width_,
        height_,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        usage
    );
    gbuffer_.albedo = memory_pool_->allocate_image(
        width_,
        height_,
        VK_FORMAT_R8G8B8A8_UNORM,
        usage
    );
    gbuffer_.emission = memory_pool_->allocate_image(
        width_,
        height_,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        usage
    );
    gbuffer_.velocity = memory_pool_->allocate_image(
        width_,
        height_,
        VK_FORMAT_R16G16_SFLOAT,
        usage
    );
    gbuffer_.depth = memory_pool_->allocate_image(
        width_,
        height_,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );

    spdlog::info("Created G-buffer: {}x{}", width_, height_);
}

void GPU3DRenderer::create_shadow_maps() {
    if (!memory_pool_ || !shadow_config_.enable_csm) {
        return;
    }

    for (int i = 0; i < shadow_config_.num_cascades && i < static_cast<int>(shadow_cascades_.size()); ++i) {
        shadow_cascades_[i].depth_map = memory_pool_->allocate_image(
            shadow_config_.resolution,
            shadow_config_.resolution,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );
    }

    spdlog::info("Created shadow maps: {} cascades at {}x{}",
                 shadow_config_.num_cascades, shadow_config_.resolution, shadow_config_.resolution);
}

void GPU3DRenderer::create_voxel_grid() {
    if (!memory_pool_ || gi_config_.method != GIConfig::Method::VXGI) {
        return;
    }

    spdlog::info("VXGI enabled (voxel grid placeholder)");
}

void GPU3DRenderer::create_render_targets() {
    if (!memory_pool_) {
        return;
    }

    swapchain_images_.clear();
    swapchain_framebuffers_.clear();
    swapchain_index_ = 0;
    swapchain_present_index_ = 0;

    hdr_image_ = memory_pool_->allocate_image(
        width_,
        height_,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );
    if (use_swapchain_) {
        for (uint32_t i = 0; i < swapchain_image_count_; ++i) {
            swapchain_images_.push_back(memory_pool_->allocate_image(
                width_,
                height_,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            ));
        }
        final_image_ = {}; // not used directly when swapchain is active
    } else {
        final_image_ = memory_pool_->allocate_image(
            width_,
            height_,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        );
    }
    depth_image_ = memory_pool_->allocate_image(
        width_,
        height_,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );

    if (depth_image_.get_image() == VK_NULL_HANDLE) {
        spdlog::error("GPU3DRenderer: Failed to create depth image");
        return;
    }

    // Create MSAA render targets when using multisampling
    if (msaa_samples_ != VK_SAMPLE_COUNT_1_BIT) {
        msaa_color_image_ = memory_pool_->allocate_image(
            width_,
            height_,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
            MemoryType::DeviceLocal,
            msaa_samples_
        );
        msaa_depth_image_ = memory_pool_->allocate_image(
            width_,
            height_,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
            MemoryType::DeviceLocal,
            msaa_samples_
        );
        spdlog::info("GPU3DRenderer: Created MSAA render targets ({}x samples)", static_cast<int>(msaa_samples_));
    }

    if (post_config_.enable_taa) {
        taa_history_ = memory_pool_->allocate_image(
            width_,
            height_,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );
    }
}

void GPU3DRenderer::create_pipelines() {
    spdlog::debug("Creating GPU pipelines (triangle pipeline)");
    create_triangle_pipeline();
}

void GPU3DRenderer::create_render_pass() {
    bool has_color = (use_swapchain_ && !swapchain_images_.empty())
        ? swapchain_images_.front().get_image() != VK_NULL_HANDLE
        : final_image_.get_image() != VK_NULL_HANDLE;

    if (device_ == VK_NULL_HANDLE || !has_color) {
        return;
    }

    const bool use_msaa = (msaa_samples_ != VK_SAMPLE_COUNT_1_BIT);

    if (use_msaa) {
        // MSAA render pass: 3 attachments [MSAA color, MSAA depth, resolve color]
        VkAttachmentDescription attachments[3]{};

        // Attachment 0: MSAA color (rendered to, resolved at end of subpass)
        attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
        attachments[0].samples = msaa_samples_;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Resolved, not stored
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Attachment 1: MSAA depth
        attachments[1].format = VK_FORMAT_D32_SFLOAT;
        attachments[1].samples = msaa_samples_;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Attachment 2: Resolve target (single-sampled final image)
        attachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // Will be resolved into
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkAttachmentReference color_ref{};
        color_ref.attachment = 0;
        color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_ref{};
        depth_ref.attachment = 1;
        depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference resolve_ref{};
        resolve_ref.attachment = 2;
        resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_ref;
        subpass.pDepthStencilAttachment = &depth_ref;
        subpass.pResolveAttachments = &resolve_ref;  // Auto-resolve MSAA at end of subpass

        VkRenderPassCreateInfo rp_info{};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.attachmentCount = 3;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;

        if (vkCreateRenderPass(device_, &rp_info, nullptr, &render_pass_) != VK_SUCCESS) {
            spdlog::warn("Failed to create MSAA render pass.");
            render_pass_ = VK_NULL_HANDLE;
        }
    } else {
        // Non-MSAA render pass: 2 attachments [color, depth]
        VkAttachmentDescription attachments[2]{};
        attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

        attachments[1].format = VK_FORMAT_D32_SFLOAT;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_ref{};
        color_ref.attachment = 0;
        color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_ref{};
        depth_ref.attachment = 1;
        depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_ref;
        subpass.pDepthStencilAttachment = &depth_ref;

        VkRenderPassCreateInfo rp_info{};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.attachmentCount = 2;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;

        if (vkCreateRenderPass(device_, &rp_info, nullptr, &render_pass_) != VK_SUCCESS) {
            spdlog::warn("Failed to create minimal render pass.");
            render_pass_ = VK_NULL_HANDLE;
        }
    }
}

void GPU3DRenderer::create_framebuffers() {
    swapchain_framebuffers_.clear();
    framebuffer_ = VK_NULL_HANDLE;

    if (device_ == VK_NULL_HANDLE || render_pass_ == VK_NULL_HANDLE) {
        return;
    }

    const bool use_msaa = (msaa_samples_ != VK_SAMPLE_COUNT_1_BIT);

    // Check required images based on MSAA mode
    if (use_msaa) {
        if (msaa_color_image_.get_view() == VK_NULL_HANDLE ||
            msaa_depth_image_.get_view() == VK_NULL_HANDLE) {
            spdlog::warn("GPU3DRenderer: MSAA images not available for framebuffer creation");
            return;
        }
    } else {
        if (depth_image_.get_view() == VK_NULL_HANDLE) {
            return;
        }
    }

    auto make_framebuffer = [&](VkImageView resolve_view) -> VkFramebuffer {
        VkFramebufferCreateInfo fb_info{};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = render_pass_;
        fb_info.width = width_;
        fb_info.height = height_;
        fb_info.layers = 1;

        VkFramebuffer fb = VK_NULL_HANDLE;

        if (use_msaa) {
            // MSAA: 3 attachments [msaa_color, msaa_depth, resolve_color]
            VkImageView attachments[] = {
                msaa_color_image_.get_view(),
                msaa_depth_image_.get_view(),
                resolve_view
            };
            fb_info.attachmentCount = 3;
            fb_info.pAttachments = attachments;

            if (vkCreateFramebuffer(device_, &fb_info, nullptr, &fb) != VK_SUCCESS) {
                spdlog::warn("Failed to create MSAA framebuffer for GPU3DRenderer.");
                fb = VK_NULL_HANDLE;
            }
        } else {
            // Non-MSAA: 2 attachments [color, depth]
            VkImageView attachments[] = {resolve_view, depth_image_.get_view()};
            fb_info.attachmentCount = 2;
            fb_info.pAttachments = attachments;

            if (vkCreateFramebuffer(device_, &fb_info, nullptr, &fb) != VK_SUCCESS) {
                spdlog::warn("Failed to create framebuffer for GPU3DRenderer.");
                fb = VK_NULL_HANDLE;
            }
        }
        return fb;
    };

    if (use_swapchain_ && !swapchain_images_.empty()) {
        swapchain_framebuffers_.reserve(swapchain_images_.size());
        for (const auto& image : swapchain_images_) {
            if (image.get_view() == VK_NULL_HANDLE) {
                swapchain_framebuffers_.push_back(VK_NULL_HANDLE);
                continue;
            }
            swapchain_framebuffers_.push_back(make_framebuffer(image.get_view()));
        }
        if (!swapchain_framebuffers_.empty()) {
            framebuffer_ = swapchain_framebuffers_.front();
        }
    } else if (final_image_.get_view() != VK_NULL_HANDLE) {
        framebuffer_ = make_framebuffer(final_image_.get_view());
    }
}

void GPU3DRenderer::create_triangle_pipeline() {
    if (device_ == VK_NULL_HANDLE || render_pass_ == VK_NULL_HANDLE) {
        return;
    }

    auto load_spirv = [](const std::vector<std::filesystem::path>& candidates) -> std::vector<uint32_t> {
        for (const auto& path : candidates) {
            if (!std::filesystem::exists(path)) continue;
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) continue;
            size_t size = static_cast<size_t>(file.tellg());
            std::vector<uint32_t> data((size + 3) / 4);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(data.data()), size);
            return data;
        }
        return {};
    };

    std::vector<std::filesystem::path> vert_candidates = {
        "shaders/vertex/simple_triangle.vert.spv",
        "build/shaders/vertex/simple_triangle.vert.spv",
        "../shaders/vertex/simple_triangle.vert.spv"
    };
    std::vector<std::filesystem::path> frag_candidates = {
        "shaders/fragment/simple_triangle.frag.spv",
        "build/shaders/fragment/simple_triangle.frag.spv",
        "../shaders/fragment/simple_triangle.frag.spv"
    };

    auto vert_spv = load_spirv(vert_candidates);
    auto frag_spv = load_spirv(frag_candidates);

    if (vert_spv.empty() || frag_spv.empty()) {
        spdlog::warn("Failed to load triangle shaders. Ensure shaders are built (simple_triangle.*).");
        return;
    }

    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    module_info.codeSize = vert_spv.size() * sizeof(uint32_t);
    module_info.pCode = vert_spv.data();
    VkShaderModule vert_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &vert_module) != VK_SUCCESS) {
        spdlog::warn("Failed to create vertex shader module.");
        return;
    }

    module_info.codeSize = frag_spv.size() * sizeof(uint32_t);
    module_info.pCode = frag_spv.data();
    VkShaderModule frag_module;
    if (vkCreateShaderModule(device_, &module_info, nullptr, &frag_module) != VK_SUCCESS) {
        spdlog::warn("Failed to create fragment shader module.");
        vkDestroyShaderModule(device_, vert_module, nullptr);
        return;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert_module;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag_module;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(math::Vec3);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribute{};
    attribute.binding = 0;
    attribute.location = 0;
    attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = 1;
    vertex_input.pVertexAttributeDescriptions = &attribute;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width_);
    viewport.height = static_cast<float>(height_);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {width_, height_};

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;  // Enabled
    raster.frontFace = VK_FRONT_FACE_CLOCKWISE;  // CW due to Y-flip in ortho projection
    raster.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = msaa_samples_;  // Use configured MSAA sample count

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_attachment{};
    color_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                      VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT |
                                      VK_COLOR_COMPONENT_A_BIT;
    color_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments = &color_attachment;

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push_range.offset = 0;
    struct PushConstantSizeHelper {
        math::Mat4 mvp;
        math::Vec4 color;
    };
    push_range.size = sizeof(PushConstantSizeHelper);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_range;

    if (vkCreatePipelineLayout(device_, &layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        spdlog::warn("Failed to create pipeline layout.");
        vkDestroyShaderModule(device_, vert_module, nullptr);
        vkDestroyShaderModule(device_, frag_module, nullptr);
        return;
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &raster;
    pipeline_info.pMultisampleState = &multisample;
    pipeline_info.pColorBlendState = &color_blend;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &triangle_pipeline_) != VK_SUCCESS) {
        spdlog::warn("Failed to create triangle pipeline.");
        vkDestroyShaderModule(device_, vert_module, nullptr);
        vkDestroyShaderModule(device_, frag_module, nullptr);
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
        triangle_pipeline_ = VK_NULL_HANDLE;
        return;
    }

    vkDestroyShaderModule(device_, vert_module, nullptr);
    vkDestroyShaderModule(device_, frag_module, nullptr);
}

void GPU3DRenderer::upload_batched_buffers(
    const std::vector<math::Vec3>& vertices,
    const std::vector<uint32_t>& indices,
    GPUBuffer& vertex_buffer,
    GPUBuffer& index_buffer,
    VkDeviceSize& vertex_capacity,
    VkDeviceSize& index_capacity
) {
    if (!memory_pool_) {
        return;
    }

    VkDeviceSize vb_size = vertices.size() * sizeof(math::Vec3);
    VkDeviceSize ib_size = indices.size() * sizeof(uint32_t);
    if (vb_size == 0 || ib_size == 0) {
        return;
    }

    auto ensure_buffer = [&](GPUBuffer& buffer,
                             VkDeviceSize required,
                             VkBufferUsageFlags usage,
                             VkDeviceSize& capacity) {
        if (!buffer.get_buffer() || capacity < required) {
            VkDeviceSize new_size = required;
            if (capacity > 0) {
                new_size = std::max(required, capacity * 2);
            }
            buffer = memory_pool_->allocate_buffer(
                new_size,
                usage,
                MemoryType::HostVisible,
                MemoryUsage::Dynamic
            );
            capacity = buffer.get_size();
        }
    };

    ensure_buffer(vertex_buffer, vb_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertex_capacity);
    ensure_buffer(index_buffer, ib_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, index_capacity);

    if (auto* vptr = vertex_buffer.map()) {
        std::memcpy(vptr, vertices.data(), static_cast<size_t>(vb_size));
        vertex_buffer.unmap();
    }
    if (auto* iptr = index_buffer.map()) {
        std::memcpy(iptr, indices.data(), static_cast<size_t>(ib_size));
        index_buffer.unmap();
    }
}

void GPU3DRenderer::destroy_resources() {
    // Shutdown text renderer first
    if (text_renderer_initialized_) {
        text_renderer_.shutdown();
        text_renderer_initialized_ = false;
    }

    gbuffer_ = {};
    voxel_grid_ = {};
    voxel_mipmaps_.clear();
    hdr_image_ = {};
    final_image_ = {};
    taa_history_ = {};
    bloom_mips_.clear();

    if (triangle_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, triangle_pipeline_, nullptr);
        triangle_pipeline_ = VK_NULL_HANDLE;
    }
    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
    for (auto fb : swapchain_framebuffers_) {
        if (fb != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device_, fb, nullptr);
        }
    }
    swapchain_framebuffers_.clear();
    if (framebuffer_ != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device_, framebuffer_, nullptr);
        framebuffer_ = VK_NULL_HANDLE;
    }
    if (render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_, render_pass_, nullptr);
        render_pass_ = VK_NULL_HANDLE;
    }
}

void GPU3DRenderer::calculate_cascade_splits(
    const Camera& /*camera*/,
    const math::Vec3& /*light_dir*/
) {
}

math::Mat4 GPU3DRenderer::calculate_light_view_proj(
    const Camera& /*camera*/,
    const math::Vec3& /*light_dir*/,
    float /*near_z*/,
    float /*far_z*/
) {
    return math::Mat4(1.0f);
}

const GPUImage& GPU3DRenderer::get_final_image() const {
    if (use_swapchain_ && !swapchain_images_.empty()) {
        uint32_t idx = std::min<uint32_t>(swapchain_present_index_, static_cast<uint32_t>(swapchain_images_.size() - 1));
        return swapchain_images_[idx];
    }
    return final_image_;
}

void GPU3DRenderer::render_indirect(
    VkCommandBuffer cmd,
    Scene& scene,
    Camera& /*camera*/
) {
    if (!indirect_rendering_enabled_) {
        spdlog::debug("Indirect rendering not enabled");
        return;
    }

    // Initialize indirect renderer if needed
    if (!indirect_renderer_initialized_ && memory_pool_) {
        indirect_renderer_.initialize(
            device_,
            physical_device_,
            *memory_pool_,
            graphics_queue_family_,
            graphics_queue_family_  // Using same queue for compute
        );
        indirect_renderer_initialized_ = indirect_renderer_.is_initialized();
        if (indirect_renderer_initialized_) {
            spdlog::info("Indirect renderer initialized for GPU3DRenderer");
        }
    }

    if (!indirect_renderer_initialized_) {
        spdlog::warn("Indirect renderer not initialized");
        return;
    }

    // Begin frame
    indirect_renderer_.begin_frame();

    // Get mobjects from scene and submit to indirect renderer
    const auto& mobjects = scene.get_mobjects();
    if (!mobjects.empty()) {
        indirect_renderer_.submit_objects(mobjects);
    }

    // Build view-projection matrix (simplified for now)
    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    float world_height = 4.0f;
    float world_width = world_height * aspect;
    math::Mat4 view_proj = glm::ortho(
        -world_width * 0.5f, world_width * 0.5f,
        -world_height * 0.5f, world_height * 0.5f,
        -10.0f, 10.0f
    );

    // Execute GPU culling and generate draw commands
    indirect_renderer_.cull_and_generate_draws(view_proj, cmd);

    // Now we need to be in a render pass to actually draw
    // The caller should have set up the render pass
    // We just bind the pipeline, geometry, and issue indirect draws

    if (triangle_pipeline_) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_pipeline_);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(width_);
        viewport.height = static_cast<float>(height_);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {width_, height_};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // Bind merged geometry buffers from indirect renderer
        indirect_renderer_.bind_geometry_buffers(cmd);

        // Record indirect draw calls
        indirect_renderer_.record_draws(cmd);

        spdlog::debug("Indirect rendering executed: {} objects submitted",
                      mobjects.size());
    }
}

}  // namespace manim

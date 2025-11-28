#include "manim/culling/gpu_culling_pipeline.hpp"
#include "manim/mobject/mobject.hpp"
#include <spdlog/spdlog.h>
#include <chrono>

namespace manim {
namespace culling {

GPUCullingPipeline::GPUCullingPipeline() = default;

GPUCullingPipeline::~GPUCullingPipeline() {
    cleanup();
}

GPUCullingPipeline::GPUCullingPipeline(GPUCullingPipeline&& other) noexcept
    : config_(std::move(other.config_))
    , initialized_(other.initialized_)
    , frustum_culler_(std::move(other.frustum_culler_))
    , contribution_culler_(std::move(other.contribution_culler_))
    , occlusion_culler_(std::move(other.occlusion_culler_))
    , software_rasterizer_(std::move(other.software_rasterizer_))
    , cluster_culler_(std::move(other.cluster_culler_))
    , device_(other.device_)
    , physical_device_(other.physical_device_)
    , memory_pool_(other.memory_pool_)
    , compute_queue_family_(other.compute_queue_family_)
    , object_count_(other.object_count_)
    , screen_width_(other.screen_width_)
    , screen_height_(other.screen_height_)
    , current_frustum_(other.current_frustum_)
    , current_view_proj_(other.current_view_proj_)
    , stats_(other.stats_)
    , cached_bounds_(std::move(other.cached_bounds_))
    , frustum_visible_cache_(std::move(other.frustum_visible_cache_))
{
    other.initialized_ = false;
    other.device_ = VK_NULL_HANDLE;
}

GPUCullingPipeline& GPUCullingPipeline::operator=(GPUCullingPipeline&& other) noexcept {
    if (this != &other) {
        cleanup();

        config_ = std::move(other.config_);
        initialized_ = other.initialized_;
        frustum_culler_ = std::move(other.frustum_culler_);
        contribution_culler_ = std::move(other.contribution_culler_);
        occlusion_culler_ = std::move(other.occlusion_culler_);
        software_rasterizer_ = std::move(other.software_rasterizer_);
        cluster_culler_ = std::move(other.cluster_culler_);
        device_ = other.device_;
        physical_device_ = other.physical_device_;
        memory_pool_ = other.memory_pool_;
        compute_queue_family_ = other.compute_queue_family_;
        object_count_ = other.object_count_;
        screen_width_ = other.screen_width_;
        screen_height_ = other.screen_height_;
        current_frustum_ = other.current_frustum_;
        current_view_proj_ = other.current_view_proj_;
        stats_ = other.stats_;
        cached_bounds_ = std::move(other.cached_bounds_);
        frustum_visible_cache_ = std::move(other.frustum_visible_cache_);

        other.initialized_ = false;
        other.device_ = VK_NULL_HANDLE;
    }
    return *this;
}

void GPUCullingPipeline::initialize(
    VkDevice device,
    VkPhysicalDevice physical_device,
    MemoryPool& memory_pool,
    uint32_t compute_queue_family,
    const CullingConfig& config
) {
    if (initialized_) {
        spdlog::warn("GPUCullingPipeline already initialized");
        return;
    }

    device_ = device;
    physical_device_ = physical_device;
    memory_pool_ = &memory_pool;
    compute_queue_family_ = compute_queue_family;
    config_ = config;

    // Initialize frustum culler
    frustum_culler_ = std::make_unique<FrustumCuller>();
    frustum_culler_->initialize(device, physical_device, memory_pool, compute_queue_family);

    if (!frustum_culler_->is_initialized()) {
        spdlog::error("Failed to initialize frustum culler");
        return;
    }

    // Initialize contribution culler
    contribution_culler_ = std::make_unique<ContributionCuller>();
    ContributionCullingConfig contrib_config;
    contrib_config.min_screen_size = config_.contribution_min_screen_size;
    contrib_config.use_distance_scaling = config_.contribution_use_distance_scaling;
    contribution_culler_->set_config(contrib_config);
    contribution_culler_->initialize(device, physical_device, memory_pool, compute_queue_family);

    if (!contribution_culler_->is_initialized()) {
        spdlog::warn("Contribution culler failed to initialize - contribution culling disabled");
        contribution_culler_.reset();
    }

    // Initialize occlusion culler
    occlusion_culler_ = std::make_unique<OcclusionCuller>();
    OcclusionCuller::HiZConfig hiz_config;
    hiz_config.depth_bias = config_.occlusion_depth_bias;
    occlusion_culler_->set_config(hiz_config);
    occlusion_culler_->initialize(device, physical_device, memory_pool, compute_queue_family);

    if (!occlusion_culler_->is_initialized()) {
        spdlog::warn("Occlusion culler failed to initialize - occlusion culling disabled");
        occlusion_culler_.reset();
    }

    // Initialize software rasterizer
    software_rasterizer_ = std::make_unique<SoftwareRasterizer>();
    SoftwareRasterizerConfig raster_config;
    raster_config.depth_buffer_width = config_.software_raster_width;
    raster_config.depth_buffer_height = config_.software_raster_height;
    raster_config.small_occluder_threshold = config_.small_occluder_threshold;
    software_rasterizer_->set_config(raster_config);
    software_rasterizer_->initialize(device, physical_device, memory_pool, compute_queue_family);

    if (!software_rasterizer_->is_initialized()) {
        spdlog::warn("Software rasterizer failed to initialize - software rasterization disabled");
        software_rasterizer_.reset();
    }

    // Initialize cluster culler
    cluster_culler_ = std::make_unique<ClusterCuller>();
    cluster_culler_->initialize(device, physical_device, memory_pool, compute_queue_family);

    if (!cluster_culler_->is_initialized()) {
        spdlog::warn("Cluster culler failed to initialize - cluster culling disabled");
        cluster_culler_.reset();
    }

    // Spatial index will be initialized in Phase 4

    initialized_ = true;
    spdlog::info("GPUCullingPipeline initialized (frustum: {}, contribution: {}, occlusion: {}, "
                 "software_raster: {}, cluster: {})",
                 frustum_culler_ != nullptr, contribution_culler_ != nullptr,
                 occlusion_culler_ != nullptr, software_rasterizer_ != nullptr,
                 cluster_culler_ != nullptr);
}

void GPUCullingPipeline::cleanup() {
    if (frustum_culler_) {
        frustum_culler_->cleanup();
        frustum_culler_.reset();
    }

    if (contribution_culler_) {
        contribution_culler_->cleanup();
        contribution_culler_.reset();
    }

    if (occlusion_culler_) {
        occlusion_culler_->cleanup();
        occlusion_culler_.reset();
    }

    if (software_rasterizer_) {
        software_rasterizer_->cleanup();
        software_rasterizer_.reset();
    }

    if (cluster_culler_) {
        cluster_culler_->cleanup();
        cluster_culler_.reset();
    }

    device_ = VK_NULL_HANDLE;
    physical_device_ = VK_NULL_HANDLE;
    memory_pool_ = nullptr;
    initialized_ = false;
}

void GPUCullingPipeline::set_config(const CullingConfig& config) {
    config_ = config;
}

void GPUCullingPipeline::update_object_bounds(
    const std::vector<std::shared_ptr<Mobject>>& mobjects
) {
    cached_bounds_.clear();
    cached_bounds_.reserve(mobjects.size());

    for (size_t i = 0; i < mobjects.size(); ++i) {
        const auto& mob = mobjects[i];
        if (!mob) continue;

        auto bbox = mob->compute_bounding_box();
        cached_bounds_.emplace_back(bbox.min, bbox.max, static_cast<uint32_t>(i));
    }

    object_count_ = static_cast<uint32_t>(cached_bounds_.size());

    // Upload to frustum culler
    if (frustum_culler_ && frustum_culler_->is_initialized()) {
        frustum_culler_->upload_object_bounds(cached_bounds_);
    }

    stats_.total_objects = object_count_;
}

void GPUCullingPipeline::update_object_bounds(const std::vector<ObjectBounds>& bounds) {
    cached_bounds_ = bounds;
    object_count_ = static_cast<uint32_t>(bounds.size());

    // Upload to frustum culler
    if (frustum_culler_ && frustum_culler_->is_initialized()) {
        frustum_culler_->upload_object_bounds(bounds);
    }

    stats_.total_objects = object_count_;
}

FrustumPlanes GPUCullingPipeline::extract_frustum_planes(const math::Mat4& view_proj) {
    return FrustumCuller::extract_frustum_planes(view_proj);
}

void GPUCullingPipeline::execute(const math::Mat4& view_proj, VkCommandBuffer cmd_buffer) {
    current_view_proj_ = view_proj;
    current_frustum_ = extract_frustum_planes(view_proj);
    execute(current_frustum_, cmd_buffer);
}

void GPUCullingPipeline::update_hiz_pyramid(
    VkImage depth_image,
    VkImageView depth_image_view,
    uint32_t width,
    uint32_t height,
    VkCommandBuffer cmd_buffer
) {
    if (!occlusion_culler_ || !occlusion_culler_->is_initialized()) {
        return;
    }

    screen_width_ = width;
    screen_height_ = height;

    occlusion_culler_->build_hiz_pyramid(depth_image, depth_image_view, width, height, cmd_buffer);
}

void GPUCullingPipeline::execute_frustum_culling(const math::Mat4& view_proj, VkCommandBuffer cmd_buffer) {
    // Same as execute for now (only frustum culling implemented)
    execute(view_proj, cmd_buffer);
}

void GPUCullingPipeline::execute(const FrustumPlanes& frustum, VkCommandBuffer cmd_buffer) {
    if (!initialized_ || object_count_ == 0) {
        stats_.frustum_visible = 0;
        stats_.occlusion_visible = 0;
        stats_.final_visible = 0;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    current_frustum_ = frustum;

    // Phase 1: Frustum culling
    if (config_.enable_frustum_culling && frustum_culler_ && frustum_culler_->is_initialized()) {
        auto frustum_start = std::chrono::high_resolution_clock::now();

        frustum_culler_->cull(frustum, cmd_buffer);

        auto frustum_end = std::chrono::high_resolution_clock::now();
        stats_.frustum_cull_time_ms = std::chrono::duration<float, std::milli>(
            frustum_end - frustum_start).count();

        stats_.frustum_visible = frustum_culler_->get_visible_count();
        stats_.final_visible = stats_.frustum_visible;

        // Cache frustum visible indices for occlusion culling
        if (config_.enable_occlusion_culling) {
            frustum_visible_cache_ = frustum_culler_->download_visible_indices();
        }
    } else {
        // No frustum culling - all objects visible
        stats_.frustum_visible = object_count_;
        stats_.final_visible = object_count_;

        // Fill cache with all indices
        if (config_.enable_occlusion_culling) {
            frustum_visible_cache_.resize(object_count_);
            for (uint32_t i = 0; i < object_count_; ++i) {
                frustum_visible_cache_[i] = i;
            }
        }
    }

    // Phase 2: Occlusion culling
    if (config_.enable_occlusion_culling && occlusion_culler_ && occlusion_culler_->is_initialized()) {
        auto occlusion_start = std::chrono::high_resolution_clock::now();

        // Run occlusion culling on frustum-visible objects
        auto occlusion_visible = occlusion_culler_->cull(
            cached_bounds_,
            frustum_visible_cache_,
            current_view_proj_,
            screen_width_,
            screen_height_,
            cmd_buffer
        );

        auto occlusion_end = std::chrono::high_resolution_clock::now();
        stats_.occlusion_cull_time_ms = std::chrono::duration<float, std::milli>(
            occlusion_end - occlusion_start).count();

        stats_.occlusion_visible = static_cast<uint32_t>(occlusion_visible.size());
        stats_.final_visible = stats_.occlusion_visible;
    } else {
        // No occlusion culling - all frustum visible objects remain visible
        stats_.occlusion_visible = stats_.frustum_visible;
    }

    auto end = std::chrono::high_resolution_clock::now();
    stats_.total_time_ms = std::chrono::duration<float, std::milli>(end - start).count();

    spdlog::debug("Culling: {} total, {} frustum, {} occlusion, {} final ({:.2f}ms)",
                 stats_.total_objects, stats_.frustum_visible,
                 stats_.occlusion_visible, stats_.final_visible, stats_.total_time_ms);
}

std::vector<uint32_t> GPUCullingPipeline::get_visible_indices() {
    if (!config_.enable_frustum_culling || !frustum_culler_) {
        // Return all indices if culling disabled
        std::vector<uint32_t> all(object_count_);
        for (uint32_t i = 0; i < object_count_; ++i) {
            all[i] = i;
        }
        return all;
    }

    return frustum_culler_->download_visible_indices();
}

uint32_t GPUCullingPipeline::get_visible_count() const {
    if (!config_.enable_frustum_culling || !frustum_culler_) {
        return object_count_;
    }
    return frustum_culler_->get_visible_count();
}

bool GPUCullingPipeline::is_visible(uint32_t object_index) const {
    if (!config_.enable_frustum_culling || !frustum_culler_) {
        return object_index < object_count_;
    }
    return frustum_culler_->is_visible(object_index);
}

const GPUBuffer& GPUCullingPipeline::get_visibility_buffer() const {
    static GPUBuffer empty;
    if (!frustum_culler_) return empty;
    return frustum_culler_->get_visibility_buffer();
}

const GPUBuffer& GPUCullingPipeline::get_visible_indices_buffer() const {
    static GPUBuffer empty;
    if (!frustum_culler_) return empty;
    return frustum_culler_->get_visible_indices_buffer();
}

} // namespace culling
} // namespace manim

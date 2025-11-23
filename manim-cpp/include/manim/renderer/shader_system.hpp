/**
 * @file shader_system.hpp
 * @brief Comprehensive shader management system
 *
 * Supports all shader types:
 * - Vertex, fragment, geometry
 * - Tessellation (control, evaluation)
 * - Compute shaders
 * - Mesh shaders (GPU-driven rendering)
 * - Ray tracing shaders (raygen, intersection, any-hit, closest-hit, miss)
 */

#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace manim {

/**
 * @brief Shader stage type
 */
enum class ShaderStage {
    Vertex,
    Fragment,
    Geometry,
    TessellationControl,
    TessellationEvaluation,
    Compute,
    Mesh,              ///< Mesh shader (Vulkan 1.3+)
    Task,              ///< Task shader (Vulkan 1.3+)
    RayGen,            ///< Ray generation
    Intersection,      ///< Ray intersection
    AnyHit,            ///< Ray any-hit
    ClosestHit,        ///< Ray closest-hit
    Miss,              ///< Ray miss
    Callable           ///< Ray callable
};

/**
 * @brief Shader source type
 */
enum class ShaderSourceType {
    GLSL,        ///< GLSL source code
    HLSL,        ///< HLSL source code
    SPIRV        ///< Pre-compiled SPIR-V
};

/**
 * @brief Shader compilation options
 */
struct ShaderCompileOptions {
    bool optimize = true;
    bool debug_info = false;
    bool warnings_as_errors = false;
    std::vector<std::string> defines;
    std::vector<std::filesystem::path> include_paths;
};

/**
 * @brief Individual shader module
 */
class ShaderModule {
public:
    ShaderModule() = default;
    ~ShaderModule();

    /**
     * @brief Create from GLSL source
     */
    void create_from_glsl(
        VkDevice device,
        const std::string& source,
        ShaderStage stage,
        const ShaderCompileOptions& options = {}
    );

    /**
     * @brief Create from SPIR-V binary
     */
    void create_from_spirv(
        VkDevice device,
        const std::vector<uint32_t>& spirv
    );

    /**
     * @brief Load from file
     */
    void load_from_file(
        VkDevice device,
        const std::filesystem::path& path,
        ShaderStage stage,
        const ShaderCompileOptions& options = {}
    );

    /**
     * @brief Destroy shader module
     */
    void destroy();

    /**
     * @brief Get Vulkan handle
     */
    VkShaderModule get_handle() const { return module_; }

    /**
     * @brief Get shader stage
     */
    ShaderStage get_stage() const { return stage_; }

    /**
     * @brief Get SPIR-V bytecode
     */
    const std::vector<uint32_t>& get_spirv() const { return spirv_; }

    /**
     * @brief Hot-reload shader
     */
    void reload();

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkShaderModule module_ = VK_NULL_HANDLE;
    ShaderStage stage_;
    std::vector<uint32_t> spirv_;
    std::filesystem::path source_path_;

    // Compile GLSL to SPIR-V
    std::vector<uint32_t> compile_glsl_to_spirv(
        const std::string& source,
        ShaderStage stage,
        const ShaderCompileOptions& options
    );
};

/**
 * @brief Graphics pipeline shader configuration
 */
struct GraphicsShaderConfig {
    std::shared_ptr<ShaderModule> vertex;
    std::shared_ptr<ShaderModule> fragment;
    std::shared_ptr<ShaderModule> geometry;        // Optional
    std::shared_ptr<ShaderModule> tess_control;    // Optional
    std::shared_ptr<ShaderModule> tess_eval;       // Optional
    std::shared_ptr<ShaderModule> mesh;            // Optional (Vulkan 1.3+)
    std::shared_ptr<ShaderModule> task;            // Optional (Vulkan 1.3+)
};

/**
 * @brief Compute shader configuration
 */
struct ComputeShaderConfig {
    std::shared_ptr<ShaderModule> compute;

    // Workgroup size
    uint32_t local_size_x = 256;
    uint32_t local_size_y = 1;
    uint32_t local_size_z = 1;
};

/**
 * @brief Ray tracing shader configuration
 */
struct RayTracingShaderConfig {
    std::shared_ptr<ShaderModule> raygen;
    std::vector<std::shared_ptr<ShaderModule>> miss;
    std::vector<std::shared_ptr<ShaderModule>> closest_hit;
    std::vector<std::shared_ptr<ShaderModule>> any_hit;      // Optional
    std::vector<std::shared_ptr<ShaderModule>> intersection;  // Optional
    std::vector<std::shared_ptr<ShaderModule>> callable;      // Optional

    uint32_t max_recursion_depth = 4;
};

/**
 * @brief Shader manager for loading and caching shaders
 */
class ShaderManager {
public:
    explicit ShaderManager(VkDevice device);
    ~ShaderManager();

    /**
     * @brief Load shader from file
     */
    std::shared_ptr<ShaderModule> load_shader(
        const std::filesystem::path& path,
        ShaderStage stage,
        const ShaderCompileOptions& options = {}
    );

    /**
     * @brief Create shader from source
     */
    std::shared_ptr<ShaderModule> create_shader(
        const std::string& source,
        ShaderStage stage,
        const std::string& name = "",
        const ShaderCompileOptions& options = {}
    );

    /**
     * @brief Get cached shader by name
     */
    std::shared_ptr<ShaderModule> get_shader(const std::string& name);

    /**
     * @brief Reload all shaders (hot-reload)
     */
    void reload_all_shaders();

    /**
     * @brief Clear shader cache
     */
    void clear_cache();

    /**
     * @brief Enable shader hot-reload
     */
    void enable_hot_reload(bool enable) { hot_reload_enabled_ = enable; }

    /**
     * @brief Check for shader file changes
     */
    void check_for_updates();

private:
    VkDevice device_;
    std::unordered_map<std::string, std::shared_ptr<ShaderModule>> shader_cache_;
    bool hot_reload_enabled_ = false;

    // File watching for hot-reload
    struct FileWatcher;
    std::unique_ptr<FileWatcher> file_watcher_;
};

/**
 * @brief Built-in shader library
 *
 * Provides pre-compiled shaders for common operations
 */
class BuiltInShaders {
public:
    static void initialize(VkDevice device);
    static void shutdown();

    // Graphics shaders
    static std::shared_ptr<ShaderModule> get_fullscreen_quad_vs();
    static std::shared_ptr<ShaderModule> get_basic_pbr_fs();
    static std::shared_ptr<ShaderModule> get_skybox_vs();
    static std::shared_ptr<ShaderModule> get_skybox_fs();

    // Compute shaders
    static std::shared_ptr<ShaderModule> get_vector_add_cs();
    static std::shared_ptr<ShaderModule> get_matrix_mul_cs();
    static std::shared_ptr<ShaderModule> get_bezier_tessellation_cs();
    static std::shared_ptr<ShaderModule> get_particle_update_cs();
    static std::shared_ptr<ShaderModule> get_cloth_simulation_cs();

    // Post-processing shaders
    static std::shared_ptr<ShaderModule> get_bloom_cs();
    static std::shared_ptr<ShaderModule> get_taa_cs();
    static std::shared_ptr<ShaderModule> get_tonemap_fs();
    static std::shared_ptr<ShaderModule> get_fxaa_fs();

    // Ray tracing shaders
    static std::shared_ptr<ShaderModule> get_shadow_raygen();
    static std::shared_ptr<ShaderModule> get_shadow_miss();
    static std::shared_ptr<ShaderModule> get_gi_raygen();
    static std::shared_ptr<ShaderModule> get_gi_closest_hit();
    static std::shared_ptr<ShaderModule> get_gi_miss();

private:
    static VkDevice device_;
    static std::unique_ptr<ShaderManager> manager_;
};

/**
 * @brief Shader reflection information
 */
struct ShaderReflection {
    struct Binding {
        uint32_t set;
        uint32_t binding;
        VkDescriptorType type;
        uint32_t count;
        std::string name;
    };

    struct PushConstant {
        uint32_t offset;
        uint32_t size;
        std::string name;
    };

    std::vector<Binding> bindings;
    std::vector<PushConstant> push_constants;

    // Vertex input (for vertex shaders)
    struct VertexAttribute {
        uint32_t location;
        VkFormat format;
        std::string name;
    };
    std::vector<VertexAttribute> vertex_attributes;

    /**
     * @brief Reflect SPIR-V bytecode
     */
    static ShaderReflection reflect(const std::vector<uint32_t>& spirv);
};

/**
 * @brief Shader cache for compiled SPIR-V binaries
 */
class ShaderCache {
public:
    explicit ShaderCache(const std::filesystem::path& cache_dir);

    /**
     * @brief Get cached SPIR-V for shader
     */
    std::vector<uint32_t> get_cached_spirv(
        const std::string& source_hash
    );

    /**
     * @brief Store SPIR-V in cache
     */
    void store_spirv(
        const std::string& source_hash,
        const std::vector<uint32_t>& spirv
    );

    /**
     * @brief Clear cache
     */
    void clear();

private:
    std::filesystem::path cache_dir_;
};

} // namespace manim

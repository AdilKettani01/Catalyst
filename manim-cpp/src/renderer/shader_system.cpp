// Shader system implementation
#include "manim/renderer/shader_system.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

namespace manim {

// ============================================================================
// ShaderModule Implementation
// ============================================================================

ShaderModule::ShaderModule(const std::filesystem::path& path, ShaderStage stage)
    : source_path_(path), stage_(stage) {
    load_from_file();
}

ShaderModule::ShaderModule(const std::string& source, ShaderStage stage)
    : stage_(stage), source_code_(source) {
    compile();
}

ShaderModule::~ShaderModule() = default;

void ShaderModule::load_from_file() {
    if (!std::filesystem::exists(source_path_)) {
        throw std::runtime_error("Shader file not found: " + source_path_.string());
    }

    std::ifstream file(source_path_, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader file: " + source_path_.string());
    }

    // Read file
    std::stringstream buffer;
    buffer << file.rdbuf();
    source_code_ = buffer.str();

    // Get last write time
    last_write_time_ = std::filesystem::last_write_time(source_path_);

    compile();
}

void ShaderModule::compile() {
    // Determine shader extension
    std::string extension;
    switch (stage_) {
        case ShaderStage::Vertex:                extension = ".vert"; break;
        case ShaderStage::Fragment:              extension = ".frag"; break;
        case ShaderStage::Geometry:              extension = ".geom"; break;
        case ShaderStage::TessellationControl:   extension = ".tesc"; break;
        case ShaderStage::TessellationEvaluation:extension = ".tese"; break;
        case ShaderStage::Compute:               extension = ".comp"; break;
        case ShaderStage::Mesh:                  extension = ".mesh"; break;
        case ShaderStage::Task:                  extension = ".task"; break;
        case ShaderStage::RayGen:                extension = ".rgen"; break;
        case ShaderStage::Intersection:          extension = ".rint"; break;
        case ShaderStage::AnyHit:                extension = ".rahit"; break;
        case ShaderStage::ClosestHit:            extension = ".rchit"; break;
        case ShaderStage::Miss:                  extension = ".rmiss"; break;
        case ShaderStage::Callable:              extension = ".rcall"; break;
    }

    // Compile GLSL to SPIR-V using glslangValidator
    std::filesystem::path temp_glsl = std::filesystem::temp_directory_path() / ("shader_temp" + extension);
    std::filesystem::path temp_spirv = std::filesystem::temp_directory_path() / "shader_temp.spv";

    // Write source to temp file
    {
        std::ofstream out(temp_glsl);
        out << source_code_;
    }

    // Compile
    std::string cmd = "glslangValidator -V -S " + std::string(extension.substr(1)) + " " +
                      temp_glsl.string() + " -o " + temp_spirv.string();

    int result = std::system(cmd.c_str());

    if (result != 0) {
        throw std::runtime_error("Shader compilation failed for: " + source_path_.string());
    }

    // Read SPIR-V
    std::ifstream spirv_file(temp_spirv, std::ios::binary);
    spirv_code_ = std::vector<uint32_t>(
        (std::istreambuf_iterator<char>(spirv_file)),
        std::istreambuf_iterator<char>()
    );

    // Convert to uint32_t
    size_t size_in_uint32 = spirv_code_.size() / sizeof(uint32_t);
    spirv_code_.resize(size_in_uint32);

    // Clean up temp files
    std::filesystem::remove(temp_glsl);
    std::filesystem::remove(temp_spirv);

    spdlog::info("Compiled shader: {} ({} bytes)", source_path_.string(), spirv_code_.size() * 4);
}

bool ShaderModule::reload_if_changed() {
    if (source_path_.empty()) {
        return false;  // No file to reload
    }

    if (!std::filesystem::exists(source_path_)) {
        return false;
    }

    auto current_time = std::filesystem::last_write_time(source_path_);
    if (current_time > last_write_time_) {
        spdlog::info("Shader changed, reloading: {}", source_path_.string());
        load_from_file();
        return true;
    }

    return false;
}

const std::vector<uint32_t>& ShaderModule::get_spirv() const {
    return spirv_code_;
}

ShaderStage ShaderModule::get_stage() const {
    return stage_;
}

const std::filesystem::path& ShaderModule::get_path() const {
    return source_path_;
}

// ============================================================================
// ShaderManager Implementation
// ============================================================================

ShaderManager::ShaderManager(const std::filesystem::path& shader_dir)
    : shader_directory_(shader_dir) {
    spdlog::info("ShaderManager initialized with directory: {}", shader_dir.string());
}

ShaderManager::~ShaderManager() = default;

std::shared_ptr<ShaderModule> ShaderManager::load_shader(
    const std::filesystem::path& path,
    ShaderStage stage
) {
    std::string key = path.string();

    // Check if already loaded
    auto it = shader_cache_.find(key);
    if (it != shader_cache_.end()) {
        return it->second;
    }

    // Load new shader
    std::filesystem::path full_path = shader_directory_ / path;
    auto shader = std::make_shared<ShaderModule>(full_path, stage);
    shader_cache_[key] = shader;

    return shader;
}

std::shared_ptr<ShaderModule> ShaderManager::create_shader_from_source(
    const std::string& source,
    ShaderStage stage,
    const std::string& name
) {
    auto shader = std::make_shared<ShaderModule>(source, stage);
    if (!name.empty()) {
        shader_cache_[name] = shader;
    }
    return shader;
}

void ShaderManager::reload_all_shaders() {
    spdlog::info("Reloading all shaders...");

    for (auto& [name, shader] : shader_cache_) {
        if (shader->reload_if_changed()) {
            spdlog::info("Reloaded shader: {}", name);
        }
    }
}

void ShaderManager::check_for_updates() {
    for (auto& [name, shader] : shader_cache_) {
        shader->reload_if_changed();
    }
}

void ShaderManager::clear_cache() {
    spdlog::info("Clearing shader cache ({} shaders)", shader_cache_.size());
    shader_cache_.clear();
}

size_t ShaderManager::get_shader_count() const {
    return shader_cache_.size();
}

// ============================================================================
// BuiltInShaders Implementation
// ============================================================================

std::shared_ptr<ShaderModule> BuiltInShaders::get_basic_pbr_vs() {
    static const std::string source = R"glsl(
#version 450
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_uv;

layout(std140, binding = 0) uniform CameraUBO {
    mat4 view_projection;
} camera;

layout(push_constant) uniform ObjectPC {
    mat4 model;
} object;

void main() {
    gl_Position = camera.view_projection * object.model * vec4(in_position, 1.0);
    out_normal = mat3(transpose(inverse(object.model))) * in_normal;
    out_uv = in_uv;
}
)glsl";

    static auto shader = std::make_shared<ShaderModule>(source, ShaderStage::Vertex);
    return shader;
}

std::shared_ptr<ShaderModule> BuiltInShaders::get_basic_pbr_fs() {
    static const std::string source = R"glsl(
#version 450
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 normal = normalize(in_normal);
    vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0));
    float diffuse = max(dot(normal, light_dir), 0.0);
    out_color = vec4(vec3(diffuse), 1.0);
}
)glsl";

    static auto shader = std::make_shared<ShaderModule>(source, ShaderStage::Fragment);
    return shader;
}

std::shared_ptr<ShaderModule> BuiltInShaders::get_bezier_tessellation_cs() {
    // Return the shader we already created in shaders/compute/bezier_tessellation.comp
    static auto shader = std::make_shared<ShaderModule>(
        std::filesystem::path("shaders/compute/bezier_tessellation.comp"),
        ShaderStage::Compute
    );
    return shader;
}

std::shared_ptr<ShaderModule> BuiltInShaders::get_bloom_cs() {
    static auto shader = std::make_shared<ShaderModule>(
        std::filesystem::path("shaders/compute/bloom.comp"),
        ShaderStage::Compute
    );
    return shader;
}

std::shared_ptr<ShaderModule> BuiltInShaders::get_taa_cs() {
    static auto shader = std::make_shared<ShaderModule>(
        std::filesystem::path("shaders/compute/taa.comp"),
        ShaderStage::Compute
    );
    return shader;
}

std::shared_ptr<ShaderModule> BuiltInShaders::get_shadow_raygen() {
    static auto shader = std::make_shared<ShaderModule>(
        std::filesystem::path("shaders/raytracing/shadow.rgen"),
        ShaderStage::RayGen
    );
    return shader;
}

std::shared_ptr<ShaderModule> BuiltInShaders::get_shadow_miss() {
    static auto shader = std::make_shared<ShaderModule>(
        std::filesystem::path("shaders/raytracing/shadow.rmiss"),
        ShaderStage::Miss
    );
    return shader;
}

}  // namespace manim

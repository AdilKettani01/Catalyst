#include "manim/cli/config.hpp"
#include <iostream>
#include <fstream>

namespace manim {

bool RenderConfig::loadFromFile(const std::filesystem::path& path) {
    // TODO: Implement TOML/JSON loading
    std::cout << "Loading config from: " << path << std::endl;
    return true;
}

bool RenderConfig::saveToFile(const std::filesystem::path& path) const {
    // TODO: Implement TOML/JSON saving
    std::cout << "Saving config to: " << path << std::endl;
    return true;
}

void RenderConfig::applyQualityPreset(QualityPreset preset) {
    quality = preset;

    switch (preset) {
        case QualityPreset::LOW:
            width = 1280; height = 720;
            frameRate = 30;
            shadowMapResolution = 1024;
            enableRayTracing = false;
            enableGI = false;
            antiAliasing = AAMode::FXAA;
            break;

        case QualityPreset::MEDIUM:
            width = 1920; height = 1080;
            frameRate = 60;
            shadowMapResolution = 2048;
            enableRayTracing = false;
            enableGI = false;
            antiAliasing = AAMode::MSAA_2X;
            break;

        case QualityPreset::HIGH:
            width = 1920; height = 1080;
            frameRate = 60;
            shadowMapResolution = 2048;
            enableRayTracing = true;
            enableGI = true;
            antiAliasing = AAMode::TAA;
            break;

        case QualityPreset::ULTRA:
            width = 3840; height = 2160;
            frameRate = 60;
            shadowMapResolution = 4096;
            enableRayTracing = true;
            enableGI = true;
            antiAliasing = AAMode::TAA;
            break;

        default:
            break;
    }
}

bool RenderConfig::validate() const {
    if (width <= 0 || height <= 0) return false;
    if (frameRate <= 0) return false;
    return true;
}

std::string RenderConfig::toString() const {
    std::stringstream ss;
    ss << "Render Configuration:\n";
    ss << "  Resolution: " << width << "x" << height << " @ " << frameRate << " fps\n";
    ss << "  Quality: " << getQualityName(quality) << "\n";
    ss << "  Backend: " << getBackendName(backend) << "\n";
    ss << "  Ray Tracing: " << (enableRayTracing ? "On" : "Off") << "\n";
    ss << "  Global Illumination: " << (enableGI ? "On" : "Off") << "\n";
    return ss.str();
}

void RenderConfig::merge(const RenderConfig& other) {
    // Merge configs - other takes precedence
    backend = other.backend;
    quality = other.quality;
    // ... merge all fields
}

RenderConfig& getGlobalConfig() {
    static RenderConfig config;
    return config;
}

std::string getBackendName(GPUBackend backend) {
    switch (backend) {
        case GPUBackend::AUTO: return "Auto";
        case GPUBackend::VULKAN: return "Vulkan";
        case GPUBackend::OPENGL: return "OpenGL";
        case GPUBackend::DIRECTX12: return "DirectX 12";
        case GPUBackend::METAL: return "Metal";
        case GPUBackend::CUDA: return "CUDA";
        case GPUBackend::OPENCL: return "OpenCL";
        default: return "Unknown";
    }
}

std::string getQualityName(QualityPreset preset) {
    switch (preset) {
        case QualityPreset::LOW: return "Low";
        case QualityPreset::MEDIUM: return "Medium";
        case QualityPreset::HIGH: return "High";
        case QualityPreset::ULTRA: return "Ultra";
        case QualityPreset::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

} // namespace manim

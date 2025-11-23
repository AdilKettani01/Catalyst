cmake_minimum_required(VERSION 3.20)
project(ManimCPP VERSION 0.1.0 LANGUAGES CXX C)

# ==============================================================================
# Project Configuration
# ==============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==============================================================================
# Options
# ==============================================================================

option(MANIM_BUILD_TESTS "Build tests" ON)
option(MANIM_BUILD_BENCHMARKS "Build benchmarks" ON)
option(MANIM_BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(MANIM_ENABLE_VULKAN "Enable Vulkan renderer" ON)
option(MANIM_ENABLE_OPENGL "Enable OpenGL renderer" ON)
option(MANIM_ENABLE_DIRECTX12 "Enable DirectX 12 renderer (Windows only)" OFF)
option(MANIM_ENABLE_METAL "Enable Metal renderer (macOS only)" OFF)
option(MANIM_ENABLE_CUDA "Enable CUDA acceleration" OFF)
option(MANIM_ENABLE_OPENCL "Enable OpenCL acceleration" OFF)
option(MANIM_ENABLE_RAYTRACING "Enable ray tracing support" ON)

# ==============================================================================
# Compiler Flags
# ==============================================================================

if(MSVC)
    add_compile_options(/W4 /WX- /permissive- /Zc:__cplusplus)
    add_compile_options(/arch:AVX2)  # SIMD support
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /Ob2 /Oi /Ot /GL)
        add_link_options(/LTCG /OPT:REF /OPT:ICF)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-march=native -mavx2)  # SIMD support
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -flto)
        add_link_options(-flto)
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# ==============================================================================
# Platform Detection
# ==============================================================================

if(WIN32)
    add_definitions(-DMANIM_PLATFORM_WINDOWS)
    set(MANIM_ENABLE_DIRECTX12 ON CACHE BOOL "" FORCE)
elseif(APPLE)
    add_definitions(-DMANIM_PLATFORM_MACOS)
    set(MANIM_ENABLE_METAL ON CACHE BOOL "" FORCE)
elseif(UNIX)
    add_definitions(-DMANIM_PLATFORM_LINUX)
endif()

# ==============================================================================
# Dependencies
# ==============================================================================

# CMake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# GLM (header-only)
find_package(glm REQUIRED)

# Eigen (header-only)
find_package(Eigen3 REQUIRED)

# spdlog (logging)
find_package(spdlog REQUIRED)

# CLI11 (command-line parsing)
find_package(CLI11 REQUIRED)

# toml++ (config parsing)
find_package(tomlplusplus REQUIRED)

# FFmpeg (video encoding)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat avutil swscale)

# FreeType (font rendering)
find_package(Freetype REQUIRED)

# HarfBuzz (text shaping)
find_package(HarfBuzz REQUIRED)

# stb (image loading)
find_path(STB_INCLUDE_DIRS "stb_image.h")

# Optional: CUDA
if(MANIM_ENABLE_CUDA)
    find_package(CUDAToolkit)
    if(CUDAToolkit_FOUND)
        add_definitions(-DMANIM_ENABLE_CUDA)
    else()
        message(WARNING "CUDA requested but not found")
        set(MANIM_ENABLE_CUDA OFF)
    endif()
endif()

# Optional: OpenCL
if(MANIM_ENABLE_OPENCL)
    find_package(OpenCL)
    if(OpenCL_FOUND)
        add_definitions(-DMANIM_ENABLE_OPENCL)
    else()
        message(WARNING "OpenCL requested but not found")
        set(MANIM_ENABLE_OPENCL OFF)
    endif()
endif()

# ==============================================================================
# Shader Compilation
# ==============================================================================

# Find glslangValidator for SPIR-V compilation
find_program(GLSLANG_VALIDATOR glslangValidator HINTS $ENV{VULKAN_SDK}/bin)
if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found")
endif()

# Function to compile shaders to SPIR-V
function(compile_shader TARGET SHADER)
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    get_filename_component(SHADER_EXT ${SHADER} EXT)

    # Determine shader stage
    if(SHADER_EXT STREQUAL ".vert")
        set(STAGE "vert")
    elseif(SHADER_EXT STREQUAL ".frag")
        set(STAGE "frag")
    elseif(SHADER_EXT STREQUAL ".comp")
        set(STAGE "comp")
    elseif(SHADER_EXT STREQUAL ".geom")
        set(STAGE "geom")
    elseif(SHADER_EXT STREQUAL ".tesc")
        set(STAGE "tesc")
    elseif(SHADER_EXT STREQUAL ".tese")
        set(STAGE "tese")
    elseif(SHADER_EXT STREQUAL ".rgen")
        set(STAGE "rgen")
    elseif(SHADER_EXT STREQUAL ".rchit")
        set(STAGE "rchit")
    elseif(SHADER_EXT STREQUAL ".rmiss")
        set(STAGE "rmiss")
    else()
        message(FATAL_ERROR "Unknown shader type: ${SHADER_EXT}")
    endif()

    set(SPIRV_OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv)

    add_custom_command(
        OUTPUT ${SPIRV_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -S ${STAGE} ${SHADER} -o ${SPIRV_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER_NAME}"
    )

    target_sources(${TARGET} PRIVATE ${SPIRV_OUTPUT})
endfunction()

# ==============================================================================
# Main Library
# ==============================================================================

# Collect source files
file(GLOB_RECURSE MANIM_SOURCES
    "src/core/*.cpp"
    "src/renderer/*.cpp"
    "src/mobject/*.cpp"
    "src/animation/*.cpp"
    "src/scene/*.cpp"
    "src/camera/*.cpp"
    "src/utils/*.cpp"
)

# Create main library
add_library(manim ${MANIM_SOURCES})

# Include directories
target_include_directories(manim
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(manim
    PUBLIC
        Vulkan::Vulkan
        glfw
        glm::glm
        Eigen3::Eigen
        spdlog::spdlog
        CLI11::CLI11
        tomlplusplus::tomlplusplus
        ${FFMPEG_LIBRARIES}
        Freetype::Freetype
        ${HARFBUZZ_LIBRARIES}
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(manim PRIVATE d3d12 dxgi d3dcompiler)
elseif(APPLE)
    target_link_libraries(manim PRIVATE "-framework Metal" "-framework MetalKit")
endif()

# CUDA support
if(MANIM_ENABLE_CUDA)
    target_link_libraries(manim PRIVATE CUDA::cudart CUDA::cuda_driver)
endif()

# OpenCL support
if(MANIM_ENABLE_OPENCL)
    target_link_libraries(manim PRIVATE OpenCL::OpenCL)
endif()

# Compile shaders
file(GLOB_RECURSE SHADER_SOURCES
    "shaders/*.vert"
    "shaders/*.frag"
    "shaders/*.comp"
    "shaders/*.geom"
    "shaders/*.tesc"
    "shaders/*.tese"
)

if(MANIM_ENABLE_RAYTRACING)
    file(GLOB_RECURSE RT_SHADER_SOURCES
        "shaders/*.rgen"
        "shaders/*.rchit"
        "shaders/*.rmiss"
    )
    list(APPEND SHADER_SOURCES ${RT_SHADER_SOURCES})
endif()

foreach(SHADER ${SHADER_SOURCES})
    compile_shader(manim ${SHADER})
endforeach()

# ==============================================================================
# Executable
# ==============================================================================

add_executable(manim-cli src/cli/main.cpp)
target_link_libraries(manim-cli PRIVATE manim)
set_target_properties(manim-cli PROPERTIES OUTPUT_NAME manim)

# ==============================================================================
# Tests
# ==============================================================================

if(MANIM_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)

    file(GLOB_RECURSE TEST_SOURCES "tests/unit/*.cpp")

    add_executable(manim_tests ${TEST_SOURCES})
    target_link_libraries(manim_tests
        PRIVATE
            manim
            GTest::gtest
            GTest::gtest_main
    )

    add_test(NAME manim_tests COMMAND manim_tests)

    # Validation tests
    file(GLOB VALIDATION_SOURCES "tests/validation/*.cpp")

    # System verification test
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/validation/system_verification.cpp")
        add_executable(system_verification tests/validation/system_verification.cpp)
        target_link_libraries(system_verification
            PRIVATE
                manim
                GTest::gtest
                GTest::gtest_main
        )
        add_test(NAME system_verification COMMAND system_verification)
    endif()

    # GPU 3D integration tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/validation/gpu_3d_integration_tests.cpp")
        add_executable(gpu_3d_integration_tests tests/validation/gpu_3d_integration_tests.cpp)
        target_link_libraries(gpu_3d_integration_tests
            PRIVATE
                manim
                GTest::gtest
                GTest::gtest_main
        )
        add_test(NAME gpu_3d_integration_tests COMMAND gpu_3d_integration_tests)
    endif()

    # Performance benchmarks (validation version)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/validation/performance_benchmarks.cpp")
        find_package(benchmark QUIET)
        if(benchmark_FOUND)
            add_executable(performance_benchmarks tests/validation/performance_benchmarks.cpp)
            target_link_libraries(performance_benchmarks
                PRIVATE
                    manim
                    benchmark::benchmark
            )
        endif()
    endif()
endif()

# ==============================================================================
# Benchmarks
# ==============================================================================

if(MANIM_BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)

    file(GLOB_RECURSE BENCHMARK_SOURCES "tests/benchmarks/*.cpp")

    add_executable(manim_benchmarks ${BENCHMARK_SOURCES})
    target_link_libraries(manim_benchmarks
        PRIVATE
            manim
            benchmark::benchmark
            benchmark::benchmark_main
    )
endif()

# ==============================================================================
# Python Bindings
# ==============================================================================

if(MANIM_BUILD_PYTHON_BINDINGS)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    find_package(pybind11 REQUIRED)

    # Create Python module named 'manim_cpp'
    pybind11_add_module(manim_cpp bindings/python/bindings.cpp)
    target_link_libraries(manim_cpp PRIVATE manim)

    # Set output directory to match Python package structure
    set_target_properties(manim_cpp PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/manim_cpp"
        OUTPUT_NAME "manim_cpp"
    )

    # Copy __init__.py to output directory
    add_custom_command(TARGET manim_cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/bindings/python/__init__.py
            ${CMAKE_BINARY_DIR}/python/manim_cpp/__init__.py
        COMMENT "Copying Python __init__.py"
    )

    # Install Python module
    install(TARGETS manim_cpp
        LIBRARY DESTINATION ${Python3_SITELIB}/manim_cpp
    )
    install(FILES bindings/python/__init__.py
        DESTINATION ${Python3_SITELIB}/manim_cpp
    )
endif()

# ==============================================================================
# Installation
# ==============================================================================

install(TARGETS manim manim-cli
    EXPORT ManimTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/manim
    DESTINATION include
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders
    DESTINATION share/manim
)

# Export targets
install(EXPORT ManimTargets
    FILE ManimTargets.cmake
    NAMESPACE Manim::
    DESTINATION lib/cmake/Manim
)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "")
message(STATUS "Manim C++ Configuration Summary")
message(STATUS "================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Renderers:")
message(STATUS "  Vulkan:     ${MANIM_ENABLE_VULKAN}")
message(STATUS "  OpenGL:     ${MANIM_ENABLE_OPENGL}")
message(STATUS "  DirectX 12: ${MANIM_ENABLE_DIRECTX12}")
message(STATUS "  Metal:      ${MANIM_ENABLE_METAL}")
message(STATUS "")
message(STATUS "Acceleration:")
message(STATUS "  CUDA:       ${MANIM_ENABLE_CUDA}")
message(STATUS "  OpenCL:     ${MANIM_ENABLE_OPENCL}")
message(STATUS "  Ray Tracing: ${MANIM_ENABLE_RAYTRACING}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  Tests:      ${MANIM_BUILD_TESTS}")
message(STATUS "  Benchmarks: ${MANIM_BUILD_BENCHMARKS}")
message(STATUS "  Python:     ${MANIM_BUILD_PYTHON_BINDINGS}")
message(STATUS "")

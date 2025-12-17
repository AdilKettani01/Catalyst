cmake_minimum_required(VERSION 3.20)
project(Catalyst VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find GLFW
find_package(glfw3 REQUIRED)

# Add MicroTeX for LaTeX rendering
add_subdirectory(lib/MicroTeX)

# Find Cairo and Pango for math rendering
find_package(PkgConfig REQUIRED)
pkg_check_modules(CairoMM REQUIRED IMPORTED_TARGET cairomm-1.0)
pkg_check_modules(PangoMM REQUIRED IMPORTED_TARGET pangomm-1.4)

# Eigen (header-only) for VID-style math utilities
find_package(Eigen3 REQUIRED)

# ============================================
# Shader compilation (must be before targets)
# ============================================
find_program(GLSLANG_VALIDATOR glslangValidator)
if(GLSLANG_VALIDATOR)
    message(STATUS "Found glslangValidator: ${GLSLANG_VALIDATOR}")
endif()

file(GLOB SHADER_SOURCES
    "${CMAKE_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/shaders/*.comp"
)

if(SHADER_SOURCES AND GLSLANG_VALIDATOR)
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPIRV_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPIRV_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
            COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o ${SPIRV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
        )
        list(APPEND SPIRV_BINARIES ${SPIRV_OUTPUT})
    endforeach()

    add_custom_target(shaders DEPENDS ${SPIRV_BINARIES})
endif()

# ============================================
# Main executable (demo)
# ============================================
# Build installable library
add_library(CatalystLib SHARED main.cpp)
add_library(Catalyst::Catalyst ALIAS CatalystLib)

target_compile_definitions(CatalystLib PRIVATE CATALYST_LIBRARY_BUILD)

# MicroTeX (LaTeX) is built as a static library and linked into our shared library.
# Ensure it is built with PIC so linking into a shared object works reliably.
set_property(TARGET LaTeX PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(CatalystLib
    PUBLIC
        Eigen3::Eigen
    PRIVATE
        Vulkan::Vulkan
        glfw
        LaTeX
        PkgConfig::CairoMM
        PkgConfig::PangoMM
        ${CMAKE_DL_LIBS}
)

target_include_directories(CatalystLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(CatalystLib PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/MicroTeX/src
)

set_target_properties(CatalystLib PROPERTIES
    OUTPUT_NAME Catalyst
    EXPORT_NAME Catalyst
)

add_executable(Catalyst main.cpp)

target_link_libraries(Catalyst PRIVATE
    Eigen3::Eigen
    Vulkan::Vulkan
    glfw
    LaTeX
    PkgConfig::CairoMM
    PkgConfig::PangoMM
    ${CMAKE_DL_LIBS}
)

target_include_directories(Catalyst PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/MicroTeX/src
)

if(TARGET shaders)
    add_dependencies(Catalyst shaders)
endif()

add_custom_command(TARGET Catalyst POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/font"
    "${CMAKE_BINARY_DIR}/font"
    COMMENT "Copying font directory to build"
)

# ============================================
# Auto-build all animation files in animations/
# ============================================
file(GLOB ANIMATION_SOURCES "${CMAKE_SOURCE_DIR}/animations/*.cpp")

foreach(ANIM_SOURCE ${ANIMATION_SOURCES})
    get_filename_component(ANIM_NAME ${ANIM_SOURCE} NAME_WE)

    add_executable(anim_${ANIM_NAME} ${ANIM_SOURCE})

    target_link_libraries(anim_${ANIM_NAME} PRIVATE
        Catalyst::Catalyst
    )

    target_include_directories(anim_${ANIM_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
    )

    # Depend on shaders (now defined before this loop)
    if(TARGET shaders)
        add_dependencies(anim_${ANIM_NAME} shaders)
    endif()

    # Copy font directory
    add_custom_command(TARGET anim_${ANIM_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/font"
        "${CMAKE_BINARY_DIR}/font"
    )
endforeach()

# ============================================
# Install + CMake package export
# ============================================
install(TARGETS CatalystLib
    EXPORT CatalystTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES
    ${CMAKE_SOURCE_DIR}/catalyst.h
    ${CMAKE_SOURCE_DIR}/vid_utils.h
    ${CMAKE_SOURCE_DIR}/Catalyst
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Runtime resources (shaders/fonts/MicroTeX res)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/font
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Catalyst
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Catalyst
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/lib/MicroTeX/res
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Catalyst/lib/MicroTeX
)

include(CMakePackageConfigHelpers)

set(CATALYST_CMAKE_CONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/Catalyst")

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/CatalystConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CatalystConfig.cmake
    INSTALL_DESTINATION ${CATALYST_CMAKE_CONFIG_DIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/CatalystConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CatalystConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/CatalystConfigVersion.cmake
    DESTINATION ${CATALYST_CMAKE_CONFIG_DIR}
)

install(EXPORT CatalystTargets
    FILE CatalystTargets.cmake
    NAMESPACE Catalyst::
    DESTINATION ${CATALYST_CMAKE_CONFIG_DIR}
)

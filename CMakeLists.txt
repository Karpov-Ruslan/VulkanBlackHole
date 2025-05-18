cmake_minimum_required(VERSION 3.16)

project(VulkanNonEuclidian)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -static")

set(VULKAN_IGNORE_DISCRETE_GPU OFF CACHE BOOL "Use integrated GPU for rendering")
set(VULKAN_DEBUG "DEBUG_DISABLED" CACHE STRING "Use DebugUtils for vulkan debugging and VVL")
set_property(CACHE VULKAN_DEBUG PROPERTY STRINGS
    "DEBUG_DISABLED"
    "DEBUG_NAMES"
    "DEBUG_VALIDATION_LAYERS"
)

set(BLACK_HOLE_RENDER_MODE "RAY_MARCHING_RK1" CACHE STRING "May be 'RAY_MARCHING_RK1' 'RAY_MARCHING_RK2', 'RAY_MARCHING_RK4', 'PRECOMPUTED'")
set_property(CACHE BLACK_HOLE_RENDER_MODE PROPERTY STRINGS
    "RAY_MARCHING_RK4"
    "RAY_MARCHING_RK2"
    "RAY_MARCHING_RK1"
    "PRECOMPUTED"
)

add_compile_definitions("BLACK_HOLE_${BLACK_HOLE_RENDER_MODE}")

# Vulkan Specific
add_compile_definitions("VK_NO_PROTOTYPES")

add_compile_definitions("VULKAN_${VULKAN_DEBUG}")

if(VULKAN_IGNORE_DISCRETE_GPU)
    add_compile_definitions("VULKAN_IGNORE_DISCRETE_GPU")
endif()

# Source files
add_executable(${PROJECT_NAME}
    main.cpp
    app.cpp
    utils/clock.cpp
    utils/window.cpp
    utils/camera.cpp
    my_vulkan/utils.cpp
    my_vulkan/gpu_allocator.cpp
    my_vulkan/vulkan_controller.cpp
    my_vulkan/vulkan_functions.cpp
    my_vulkan/shaders/shaders_list.cpp
    my_vulkan/core/core.cpp
    my_vulkan/core/passes/black_hole/black_hole_pass.cpp
    my_vulkan/core/passes/black_hole/black_hole_precompute_pass.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "./"
    "./my_vulkan/shaders/spv"
)

# Copy textures into build directory
add_custom_target(TEXTURE_COPY
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/textures"
        "${CMAKE_CURRENT_BINARY_DIR}/textures"
    WORKING_DIRECTORY "./"
    COMMENT "Copying textures into build folder..."
)
add_dependencies(${PROJECT_NAME} TEXTURE_COPY)

# SPIR-V files generation
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_target(SPIRV_GENERATION
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/my_vulkan/shaders/compile_shaders.py
    WORKING_DIRECTORY "./"
    COMMENT "Shaders Processing..."
)
add_dependencies(${PROJECT_NAME} SPIRV_GENERATION)

find_package(glfw3 REQUIRED)

target_link_libraries(${PROJECT_NAME} libglfw3.a)

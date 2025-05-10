cmake_minimum_required(VERSION 3.16)

project(VulkanNonEuclidian)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror")

set(VULKAN_DEBUG_NAMES OFF CACHE BOOL "Use DebugUtils for vulkan debugging")
set(VULKAN_IGNORE_DISCRETE_GPU OFF CACHE BOOL "Use integrated GPU for rendering")

# Vulkan Specific
add_compile_definitions("VK_NO_PROTOTYPES")
add_compile_definitions("VK_USE_PLATFORM_WIN32_KHR")

if(VULKAN_DEBUG_NAMES)
    add_compile_definitions("VULKAN_DEBUG")
endif()

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

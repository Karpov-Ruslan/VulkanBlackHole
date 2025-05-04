cmake_minimum_required(VERSION 3.16)

project(VulkanNonEuclidian)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -O2")

set(VULKAN_DEBUG_NAMES OFF CACHE BOOL "Use DebugUtils for vulkan debugging")

# Vulkan Specific
add_compile_definitions("VK_NO_PROTOTYPES")
add_compile_definitions("VK_USE_PLATFORM_WIN32_KHR")

if(VULKAN_DEBUG_NAMES)
    add_compile_definitions("VULKAN_DEBUG")
endif()

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
link_directories(${Vulkan_LIBRARY_DIRS})

# Source files
add_executable(${PROJECT_NAME}
    main.cpp
    my_vulkan/utils.cpp
    my_vulkan/gpu_allocator.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "./"
    "./my_vulkan/shaders/spv"
)

# SPIR-V files generation
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_target(SPIRV_GENERATION
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/my_vulkan/shaders/compile_shaders.py
    WORKING_DIRECTORY "./"
    COMMENT "Shaders Processing..."
)
add_dependencies(${PROJECT_NAME} SPIRV_GENERATION)

target_include_directories(${PROJECT_NAME} PUBLIC "D:/CLion 2024.1.4/bin/mingw/include/glfw")
target_link_directories(${PROJECT_NAME} PUBLIC "D:/CLion 2024.1.4/bin/mingw/lib/glfw")

target_link_libraries(${PROJECT_NAME} glfw3 ${Vulkan_LIBRARIES})

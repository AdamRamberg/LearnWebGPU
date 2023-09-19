# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.0...3.27)
# Project meta data
project(
    LearnWebGPU # name of the project, which will also be the name of the visual studio solution if you use it
    VERSION 0.1.0 # any version number
    LANGUAGES CXX C # programming languages used by the project
)

# Include the file that defines target_treat_all_warnings_as_errors
# (I do this because I reuse this function across many projects and
# want to keep this main file easy to read).
include(utils.cmake)

# Include glfw directory, to define the 'glfw' target
add_subdirectory(glfw)
# Include webgpu directory, to define the 'webgpu' target
add_subdirectory(webgpu)
# Helper to get surface for current OS
add_subdirectory(glfw3webgpu)

# Add main.cpp as executable
add_executable(${PROJECT_NAME} main.cpp)

# Add the dependencies to link
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw
    webgpu
    glfw3webgpu
)

# Use C++17
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# Turn all warnings on and treat them as errors so that are not
# tempted to ignore them (See utils.cmake for details).
target_treat_all_warnings_as_errors(${PROJECT_NAME})

# 1) Tell XCode how to generate scheme
# 2) Enable frame capture for GPU debugging
if(XCODE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
endif()

# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(${PROJECT_NAME})
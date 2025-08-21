cmake_minimum_required (VERSION 3.18)

include(${CMAKE_SOURCE_DIR}/CMakeScripts/slang-setup.cmake)

project (Lux)



# Collecting all headers/cpp/glsl files
file(GLOB_RECURSE HEADER_FILES "headers/*.h")
file(GLOB_RECURSE CPP_SOURCE_FILES "src/*.cpp")


# Making executable!
add_executable(Lux ${CPP_SOURCE_FILES} )

# Setting CPP Version
set_target_properties(Lux PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS YES)


# Download slang
download_slang()

# Platform specific compile definitions
if(UNIX)
  if(USE_WAYLAND)
    target_compile_definitions(Lux PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)
  else()
    target_compile_definitions(Lux PRIVATE VK_USE_PLATFORM_XLIB_KHR)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
  endif()
elseif(WIN32)
  target_compile_definitions(Lux PRIVATE VK_USE_PLATFORM_WIN32_KHR GLFW_EXPOSE_NATIVE_WIN32)
  target_compile_definitions(Lux PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  set(VOLK_STATIC_DEFINES "VK_USE_PLATFORM_WIN32_KHR")
endif()


# Adding subdirectories so that they would build by itself
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendor/volk)
add_subdirectory(vendor/fastgltf)

# Imgui building library
add_library(imgui STATIC 
                vendor/imgui/imgui.cpp
                vendor/imgui/imgui_draw.cpp
                vendor/imgui/imgui_tables.cpp
                vendor/imgui/imgui_widgets.cpp
                vendor/imgui/backends/imgui_impl_sdl3.cpp
                vendor/imgui/backends/imgui_impl_vulkan.cpp
                vendor/imgui/imgui_demo.cpp
 )
# Imgui include directories
target_include_directories(imgui PUBLIC "vendor/imgui" "vendor/SDL/include"  "vendor/volk")
# Imgui compile definitions
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
# Imgui link libraries
target_link_libraries(imgui PUBLIC volk SDL3::SDL3)


# Include directories
target_include_directories(Lux PRIVATE 
    "vendor/imgui"
    "vendor/glm"
    "vendor/VulkanMemoryAllocator/include"
    "vendor/fastgltf/include"
    "vendor/stb"
    ${SLANG_INCLUDE_DIR})


# Compile definitions
target_compile_definitions(Lux PRIVATE 
        VK_NO_PROTOTYPES
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_XYZW_ONLY 
        GLM_FORCE_QUAT_DATA_XYZW 
        GLM_FORCE_QUAT_CTOR_XYZW
)
# Copy SDL .dll into root folder
add_custom_command(TARGET Lux POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:SDL3::SDL3>"
    "$<TARGET_FILE_DIR:Lux>"
)

add_custom_target(Shaders DEPENDS ${SLANG_BINARY_DIR})
add_dependencies(Lux Shaders)

# Link directories
target_link_directories(Lux PRIVATE ${SLANG_LIBRARY_DIR})


# Linking libraries
target_link_libraries(Lux PRIVATE SDL3::SDL3 imgui volk fastgltf::fastgltf slang)


#-fsanitize=address -fsanitize=undefined remove that to make it possible to work with renderdoc, otherwise place in target libs
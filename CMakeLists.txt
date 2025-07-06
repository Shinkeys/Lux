cmake_minimum_required (VERSION 3.18)


project (Lux)



# Collecting all headers/cpp/glsl files
file(GLOB_RECURSE GLSL_SOURCE_FILES "resources/shaders/*.vert" "resources/shaders/*.frag" "resources/shaders/*.comp")
file(GLOB_RECURSE HEADER_FILES "headers/*.h")
file(GLOB_RECURSE CPP_SOURCE_FILES "src/*.cpp")


# Making executable!
add_executable(Lux ${CPP_SOURCE_FILES} )

# Setting CPP Version
set_target_properties(Lux PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS YES)


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

# Compiler for spir-v
if(UNIX)
  if(DEFINED ENV{VULKAN_SDK})
    set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/bin/glslangValidator")
  else()
    set(GLSL_VALIDATOR "glslangValidator")
  endif()
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
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
target_include_directories(Lux PRIVATE "vendor/imgui" "vendor/glm" "vendor/VulkanMemoryAllocator/include" "vendor/fastgltf/include" "vendor/stb")

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

# Make spirv files from basic glsl
# Thanks: https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/resources/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/resources/shaders"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(Lux Shaders)

# Linking libraries
target_link_libraries(Lux PRIVATE SDL3::SDL3 imgui volk fastgltf::fastgltf)


#-fsanitize=address -fsanitize=undefined remove that to make it possible to work with renderdoc, otherwise place in target libs
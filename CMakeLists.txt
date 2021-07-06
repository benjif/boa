cmake_minimum_required(VERSION 2.8.12)
project(boa)

set(CMAKE_CXX_FLAGS "-std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
#find_package(GLFW REQUIRED)

file(GLOB SOURCES
    lib/imgui/*.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_vulkan.cpp
    src/*.cpp
    src/boa/*.cpp
    src/boa/input/*.cpp
    src/boa/asset/*.cpp
)

add_executable(boa ${SOURCES})

add_definitions(
    -DVULKAN_HPP_NO_NODISCARD_WARNINGS
    -DVULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    -DGLM_FORCE_RADIANS
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE
    -DGLM_ENABLE_EXPERIMENTAL
    #-DTINYGLTF_IMPLEMENTATION
    #-DTINYGTLF_NO_EXTERNAL_IMAGE
    #-DTINYGTLF_NO_INCLUDE_STB_IMAGE
    #-DTINYGTLF_NO_INCLUDE_STB_IMAGE_WRITE
    #-DSTB_IMAGE_IMPLEMENTATION
    #-DSTB_IMAGE_WRITE_IMPLEMENTATION
    -DTINYOBJLOADER_IMPLEMENTATION
    -DFMT_HEADER_ONLY
)

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)

target_include_directories(boa
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/imgui/backends"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/imgui"
    PUBLIC "${PROJECT_SOURCE_DIR}/include"
    PUBLIC ${VULKAN_INCLUDE_DIR}
)

if(WIN32)
    target_include_directories(boa
        PUBLIC ${Vulkan_INCLUDE_DIRS}
        PUBLIC "${PROJECT_SOURCE_DIR}/lib/glfw-3.3.4.bin.WIN64/include"
        PUBLIC "${PROJECT_SOURCE_DIR}/lib/glm"
    )
    target_link_directories(boa
        PUBLIC ${Vulkan_INCLUDE_DIRS}
        PUBLIC "${PROJECT_SOURCE_DIR}/lib/glfw-3.3.4.bin.WIN64/lib-mingw-w64"
    )
    message(STATUS ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(boa
        ${Vulkan_LIBRARIES}
        -lglfw3
        -lgdi32
        -limm32
    )
else()
    target_link_libraries(boa
        ${Vulkan_LIBRARIES}
        -lglfw3
        -lX11
        -lXxf86vm
        -lpthread
        -ldl
        -lXrandr
        -lXi
    )
endif()
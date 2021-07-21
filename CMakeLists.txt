cmake_minimum_required(VERSION 2.8.12)
project(boa)

set(CMAKE_CXX_FLAGS "-std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "-march=native -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
#set(CMAKE_CXX_LINKER_FLAGS "-fsanitize=address")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
find_package(fmt REQUIRED)
find_package(Bullet REQUIRED)

find_program(CCACHE "ccache")
if(CCACHE)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE}")
endif(CCACHE)

file(GLOB SOURCES
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_vulkan.cpp
    lib/imgui/*.cpp
    src/*.cpp
    src/boa/*.cpp
    src/boa/input/*.cpp
    src/boa/ecs/*.cpp
    src/boa/gfx/*.cpp
    src/boa/gfx/vk/*.cpp
    src/boa/gfx/asset/*.cpp
    src/boa/phy/*.cpp
)

add_executable(boa ${SOURCES})

option(BENCHMARK "BENCHMARK" OFF)

if(BENCHMARK)
    add_definitions(
        -DBENCHMARK
    )
else()
    add_definitions(
        -UBENCHMARK
    )
endif(BENCHMARK)

add_definitions(
    -DVULKAN_HPP_NO_NODISCARD_WARNINGS
    -DVULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    -DGLM_FORCE_RADIANS
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE
    -DGLM_ENABLE_EXPERIMENTAL
    -DTINYOBJLOADER_IMPLEMENTATION
    -DBT_USE_DOUBLE_PRECISION=1
)

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)

target_include_directories(boa
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/imgui/backends"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/imgui"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/include"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/rapidjson/include/rapidjson"
    PUBLIC "${PROJECT_SOURCE_DIR}/include"
    PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC ${BULLET_INCLUDE_DIRS}
)

if(WIN32)
    target_include_directories(boa
        PUBLIC "${PROJECT_SOURCE_DIR}/lib/glfw-3.3.4.bin.WIN64/include"
        PUBLIC "${PROJECT_SOURCE_DIR}/lib/glm"
    )
    target_link_directories(boa
        PUBLIC "${PROJECT_SOURCE_DIR}/lib/glfw-3.3.4.bin.WIN64/lib-mingw-w64"
    )
    target_link_libraries(boa
        ${Vulkan_LIBRARIES}
        ${BULLET_LIBRARIES}
        fmt::fmt
        -lglfw3
        -lgdi32
        -limm32
    )
else()
    target_link_libraries(boa
        ${Vulkan_LIBRARIES}
        ${BULLET_LIBRARIES}
        fmt::fmt
        -lglfw3
        -lX11
        -lXxf86vm
        -lpthread
        -ldl
        -lXrandr
        -lXi
    )
endif()

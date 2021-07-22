CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(boa)

SET(CMAKE_CXX_FLAGS "-std=c++17")
SET(CMAKE_CXX_FLAGS_RELEASE "-march=native -O3 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
#set(CMAKE_CXX_LINKER_FLAGS "-fsanitize=address")
SET(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
SET(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)

LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

FIND_PACKAGE(Vulkan REQUIRED)
FIND_PACKAGE(fmt REQUIRED)
FIND_PACKAGE(Bullet REQUIRED)

FIND_PROGRAM(CCACHE "ccache")
if(CCACHE)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE}")
endif(CCACHE)

FILE(GLOB SOURCES
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
ADD_EXECUTABLE(boa ${SOURCES})

OPTION(BENCHMARK "BENCHMARK" OFF)
if(BENCHMARK)
    ADD_DEFINITIONS(
        -DBENCHMARK
    )
else()
    ADD_DEFINITIONS(
        -UBENCHMARK
    )
endif(BENCHMARK)

ADD_DEFINITIONS(
    ${BULLET_DEFINITIONS}
    -DVULKAN_HPP_NO_NODISCARD_WARNINGS
    -DVULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    -DGLM_FORCE_RADIANS
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE
    -DGLM_ENABLE_EXPERIMENTAL
    -DTINYOBJLOADER_IMPLEMENTATION
    -DBT_USE_DOUBLE_PRECISION=1
)

INCLUDE_DIRECTORIES(
    ${BULLET_INCLUDE_DIRS}
    ${}
    "${PROJECT_SOURCE_DIR}/lib/imgui/backends"
    "${PROJECT_SOURCE_DIR}/lib/imgui"
    "${PROJECT_SOURCE_DIR}/lib/include"
    "${PROJECT_SOURCE_DIR}/lib/rapidjson/include/rapidjson"
    "${PROJECT_SOURCE_DIR}/include"
)

TARGET_LINK_LIBRARIES(boa
    ${BULLET_LIBRARIES}
    fmt::fmt
    -lglfw3
)

if(WIN32)
    INCLUDE_DIRECTORIES(
        PRIVATE "${PROJECT_SOURCE_DIR}/lib/glfw-3.3.4.bin.WIN64/include"
        PRIVATE "${PROJECT_SOURCE_DIR}/lib/glm"
        PRIVATE "$ENV{VULKAN_SDK}/Include"
    )
    LINK_DIRECTORIES(
        PRIVATE "${PROJECT_SOURCE_DIR}/lib/glfw-3.3.4.bin.WIN64/lib-mingw-w64"
        PRIVATE "$ENV{VULKAN_SDK}/Lib"
    )
    TARGET_LINK_LIBRARIES(boa
        -lvulkan-1
        -lgdi32
        -limm32
    )
else()
    INCLUDE_DIRECTORIES(
        ${Vulkan_INCLUDE_DIRS}
    )
    TARGET_LINK_LIBRARIES(boa
        ${Vulkan_LIBRARIES}
        -lX11
        -lXxf86vm
        -lpthread
        -ldl
        -lXrandr
        -lXi
    )
endif()

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(boa)

SET(CMAKE_CXX_FLAGS "-std=c++17")
SET(CMAKE_CXX_FLAGS_RELEASE "-march=native -O3 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

INCLUDE(FetchContent)
FIND_PACKAGE(Vulkan REQUIRED)

IF(NOT VULKAN_FOUND)
    message(FATAL_ERROR "Failed to find Vulkan")
ENDIF()

FIND_PROGRAM(CCACHE "ccache")
IF(CCACHE)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE}")
ENDIF(CCACHE)

# ***********
# ** IMGUI **
# ***********
FetchContent_Declare(
    imgui
    GIT_REPOSITORY          "https://github.com/eliasdaler/imgui"
    GIT_TAG                 "6facc66e30e3971437c22f0646d5540bc059479c"
    GIT_PROGRESS            TRUE
    GIT_SHALLOW             TRUE
    USES_TERMINAL_DOWNLOAD  TRUE
)
FetchContent_GetProperties(imgui)
IF(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
ENDIF()
SET(IMGUI_INCLUDE_DIRS "${imgui_SOURCE_DIR}")

FILE(GLOB SOURCES
    src/*.cpp
    src/boa/*.cpp
    src/boa/input/*.cpp
    src/boa/ecs/*.cpp
    src/boa/gfx/*.cpp
    src/boa/gfx/vk/*.cpp
    src/boa/gfx/asset/*.cpp
    src/boa/phy/*.cpp
    "${IMGUI_INCLUDE_DIRS}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_INCLUDE_DIRS}/backends/imgui_impl_vulkan.cpp"
    "${IMGUI_INCLUDE_DIRS}/*.cpp"
)
ADD_EXECUTABLE(boa ${SOURCES})

OPTION(BENCHMARK "BENCHMARK" OFF)
IF(BENCHMARK)
    ADD_DEFINITIONS(-DBENCHMARK)
ELSE()
    ADD_DEFINITIONS(-UBENCHMARK)
ENDIF(BENCHMARK)

ADD_DEFINITIONS(
    -DVULKAN_HPP_NO_NODISCARD_WARNINGS
    -DVULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    -DGLM_FORCE_RADIANS
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE
    -DGLM_ENABLE_EXPERIMENTAL
    -DBT_USE_DOUBLE_PRECISION=1
)

SET(FETCHCONTENT_QUIET FALSE)

# *********
# ** FMT **
# *********
FetchContent_Declare(
    fmt
    GIT_REPOSITORY          "https://github.com/fmtlib/fmt"
    GIT_TAG                 "8.0.1"
    GIT_PROGRESS            TRUE
    GIT_SHALLOW             TRUE
    USES_TERMINAL_DOWNLOAD  TRUE
)
FetchContent_GetProperties(fmt)
SET(FMT_DOC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(fmt)
SET(FMT_INCLUDE_DIRS "${fmt_SOURCE_DIR}/include")

# ************
# **  GLFW  **
# ************
FetchContent_Declare(
    glfw
    GIT_REPOSITORY          "https://github.com/glfw/glfw"
    GIT_TAG                 "3.3.3"
    GIT_PROGRESS            TRUE
    GIT_SHALLOW             TRUE
    USES_TERMINAL_DOWNLOAD  TRUE
)
FetchContent_GetProperties(glfw)
OPTION(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
OPTION(GLFW_INSTALL "Generate installation target" OFF)
OPTION(GLFW_BUILD_TESTS "Build GLFW tests" OFF)
OPTION(GLFW_BUILD_EXAMPLES "Build GLFW examples" OFF)
SET(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
SET(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
SET(BENCHMARK_ENABLE_LTO OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
SET(GLFW_INCLUDE_DIRS "${glfw_SOURCE_DIR}/include")

# *********
# ** GLM **
# *********
FetchContent_Declare(
    glm
    GIT_REPOSITORY          "https://github.com/g-truc/glm"
    GIT_TAG                 "0.9.9.8"
    GIT_PROGRESS            TRUE
    GIT_SHALLOW             TRUE
    USES_TERMINAL_DOWNLOAD  TRUE
)
FetchContent_MakeAvailable(glm)
SET(GLM_INCLUDE_DIRS "${glm_SOURCE_DIR}")

# **************
# ** TINYGLTF **
# **************
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY          "https://github.com/syoyo/tinygltf"
    GIT_TAG                 "v2.5.0"
    GIT_PROGRESS            TRUE
    GIT_SHALLOW             TRUE
    USES_TERMINAL_DOWNLOAD  TRUE
    CONFIGURE_COMMAND       ""
    BUILD_COMMAND           ""
)
FetchContent_GetProperties(tinygltf)
IF(NOT tinygltf_POPULATED)
    FetchContent_Populate(tinygltf)
ENDIF()
SET(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
SET(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
SET(TINYGLTF_INCLUDE_DIRS "${tinygltf_SOURCE_DIR}")

# *********
# ** VMA **
# *********
FetchContent_Declare(
    vma
    GIT_REPOSITORY          "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator"
    GIT_TAG                 "v2.3.0"
    GIT_PROGRESS            TRUE
    GIT_SHALLOW             TRUE
    USES_TERMINAL_DOWNLOAD  TRUE
    CONFIGURE_COMMAND       ""
    BUILD_COMMAND           ""
)
FetchContent_GetProperties(vma)
IF(NOT vma_POPULATED)
    FetchContent_Populate(vma)
ENDIF()
SET(VMA_INCLUDE_DIRS "${vma_SOURCE_DIR}/src")

# ************
# ** BULLET **
# ************
FetchContent_Declare(
    bullet
    GIT_REPOSITORY          "https://github.com/bulletphysics/bullet3.git"
    GIT_TAG                 "2.89"
    GIT_PROGRESS            TRUE
    GIT_SHALLOW             TRUE
    USES_TERMINAL_DOWNLOAD  TRUE
)
FetchContent_GetProperties(bullet)

IF(NOT bullet_POPULATED)
    SET(USE_DOUBLE_PRECISION ON CACHE INTERNAL "" FORCE)
    SET(BUILD_UNIT_TESTS OFF CACHE INTERNAL "" FORCE)
    SET(BUILD_CPU_DEMOS OFF CACHE INTERNAL "" FORCE)
    SET(BUILD_BULLET2_DEMOS OFF CACHE INTERNAL "" FORCE)
    SET(BUILD_EXTRAS OFF CACHE INTERNAL "" FORCE)

    IF(WIN32)
        SET(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
        SET(USE_MSVC_RUNTIME_LIBRARY_DLL YES CACHE INTERNAL "" FORCE)
        SET(INSTALL_LIBS YES CACHE INTERNAL "" FORCE)
        SET(INSTALL_CMAKE_FILES YES CACHE INTERNAL "" FORCE)
    ELSE()
        SET(BUILD_SHARED_LIBS ON CACHE INTERNAL "" FORCE)
    ENDIF(WIN32)

    FetchContent_Populate(bullet)
    ADD_SUBDIRECTORY(${bullet_SOURCE_DIR} ${bullet_BINARY_DIR})
ENDIF()
SET(BULLET_INCLUDE_DIRS "${bullet_SOURCE_DIR}/src")
FetchContent_MakeAvailable(bullet)

# ***************
# ** RAPIDJSON **   (FOR TINYGLTF)
# ***************
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY          "https://github.com/tencent/rapidjson"
    GIT_TAG                 "v1.1.0"
    GIT_PROGRESS            TRUE
    GIT_SHALLOW             TRUE
    USES_TERMINAL_DOWNLOAD  TRUE
    CONFIGURE_COMMAND       ""
    BUILD_COMMAND           ""
)
FetchContent_GetProperties(rapidjson)
IF(NOT rapidjson_POPULATED)
    FetchContent_Populate(rapidjson)
ENDIF()
SET(RAPIDJSON_INCLUDE_DIRS "${rapidjson_SOURCE_DIR}/include/rapidjson")

INCLUDE_DIRECTORIES(
    "${PROJECT_SOURCE_DIR}/include"
    ${IMGUI_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${VMA_INCLUDE_DIRS}
    ${TINYGLTF_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIRS}
    ${FMT_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
)

LINK_DIRECTORIES("${PROJECT_SOURCE_DIR}/cmake/lib")

TARGET_LINK_LIBRARIES(boa
    ${Vulkan_LIBRARIES}
    fmt
    glfw
    BulletSoftBody
    BulletDynamics
    Bullet3Common
    BulletCollision
    LinearMath
)

IF(WIN32)
    TARGET_LINK_LIBRARIES(boa
        -lgdi32
        -limm32
    )
ELSE()
    TARGET_LINK_LIBRARIES(boa
        -lX11
        -lXxf86vm
        -lpthread
        -ldl
        -lXrandr
        -lXi
    )
ENDIF(WIN32)

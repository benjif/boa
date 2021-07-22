CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(boa)

SET(CMAKE_CXX_FLAGS "-std=c++17")
SET(CMAKE_CXX_FLAGS_RELEASE "-march=native -O3 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
SET(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)

LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

INCLUDE(FetchContent)
FIND_PACKAGE(Vulkan REQUIRED)
FIND_PACKAGE(fmt REQUIRED)

FIND_PROGRAM(CCACHE "ccache")
IF(CCACHE)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE}")
ENDIF(CCACHE)

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
)

SET(FETCHCONTENT_QUIET FALSE)

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
OPTION(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
OPTION(GLFW_INSTALL "Generate installation target" OFF)
#ADD_SUBDIRECTORY(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
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

    IF(WIN32)
        SET(USE_MSVC_RUNTIME_LIBRARY_DLL YES CACHE INTERNAL "" FORCE)
        SET(INSTALL_LIBS YES CACHE INTERNAL "" FORCE)
        SET(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
    ELSE()
        SET(BUILD_SHARED_LIBS ON CACHE INTERNAL "" FORCE)
    ENDIF()

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
#ADD_SUBDIRECTORY(${imgui_SOURCE_DIR})

INCLUDE_DIRECTORIES(
    "${PROJECT_SOURCE_DIR}/include"
    ${GLM_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${VMA_INCLUDE_DIRS}
    ${TINYGLTF_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIRS}
    ${IMGUI_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES(boa
    ${Vulkan_LIBRARIES}
    fmt::fmt
    LinearMath
    Bullet3Common
    BulletDynamics
    BulletSoftBody
    glfw
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

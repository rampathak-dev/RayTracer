CMAKE_MINIMUM_REQUIRED(VERSION 3.30)
PROJECT(RAYTRACER LANGUAGES CXX)

STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" ARCHITECTURE)

IF(WIN32)
    IF(NOT ARCHITECTURE MATCHES "^(x86_64|amd64)$")
        MESSAGE(FATAL_ERROR
            "Unsupported architecture for Windows: ${ARCHITECTURE}")
    ENDIF()
ELSEIF(APPLE)
    IF(NOT ARCHITECTURE MATCHES "^(x86_64|amd64|aarch64|arm64)$")
        MESSAGE(FATAL_ERROR
            "Unsupported architecture for MacOS: ${ARCHITECTURE}")
    ENDIF()
ELSEIF(UNIX AND NOT APPLE)
    IF(NOT ARCHITECTURE MATCHES "^(x86_64|amd64|aarch64|arm64)$")
        MESSAGE(FATAL_ERROR
            "Unsupported architecture for Linux: ${ARCHITECTURE}")
    ENDIF()
ELSE()
    MESSAGE(FATAL_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}")
ENDIF()

STRING(TOLOWER ${CMAKE_CXX_COMPILER_ID} COMPILER)

IF(NOT COMPILER MATCHES "^(msvc|appleclang|clang|gnu)$")
    MESSAGE(FATAL_ERROR "Unsupported compiler: ${COMPILER}")
ENDIF()

SET(SRC_ROOT "${PROJECT_SOURCE_DIR}/src")

SET(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_STANDARD_REQUIRED TRUE)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()

FILE(
    GLOB SOURCES CONFIGURE_DEPENDS
    "${SRC_ROOT}/*.cpp" "${SRC_ROOT}/headers/*.hpp")

ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES})

IF(MSVC)
    TARGET_COMPILE_OPTIONS(
        ${PROJECT_NAME} PRIVATE
        /W4
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Release>:-O2 /arch:AVX2>)
ELSE()
    TARGET_COMPILE_OPTIONS(
        ${PROJECT_NAME} PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3 -march=native>)
ENDIF()

TARGET_COMPILE_DEFINITIONS(
    ${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
    SHADER_PATH="${SRC_ROOT}/Shader.wgsl"
    EARTH_IMAGE="${SRC_ROOT}/images/Earth.jpg")

INCLUDE(CheckIPOSupported)
CHECK_IPO_SUPPORTED(RESULT IPO_SUPPORTED)

IF(IPO_SUPPORTED)
    SET_TARGET_PROPERTIES(
        ${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
ENDIF()

SET(EXTERNAL_DIR "${SRC_ROOT}/external")
SET(WGPU_DIR "${EXTERNAL_DIR}/WGPU")

TARGET_INCLUDE_DIRECTORIES(
    ${PROJECT_NAME} PRIVATE
    "${WGPU_DIR}/include" "${EXTERNAL_DIR}")

IF(WIN32)
    SET(WGPU_PLATFORM_DIR "${WGPU_DIR}/windows/x86_64")

    IF(MSVC)
        SET(WGPU_DEBUG_NAME "wgpu_native_windows_x86_64_msvc_debug.lib")
        SET(WGPU_RELEASE_NAME "wgpu_native_windows_x86_64_msvc_release.lib")

        SET(WGPU_LIB_DEBUG
            "${WGPU_PLATFORM_DIR}/msvc_debug/${WGPU_DEBUG_NAME}")
        SET(WGPU_LIB_RELEASE
            "${WGPU_PLATFORM_DIR}/msvc_release/${WGPU_RELEASE_NAME}")
    ELSE()
        SET(WGPU_DEBUG_NAME "libwgpu_native_windows_x86_64_gnu_debug.a")
        SET(WGPU_RELEASE_NAME "libwgpu_native_windows_x86_64_gnu_release.a")

        SET(WGPU_LIB_DEBUG
            "${WGPU_PLATFORM_DIR}/gnu_debug/${WGPU_DEBUG_NAME}")
        SET(WGPU_LIB_RELEASE
            "${WGPU_PLATFORM_DIR}/gnu_release/${WGPU_RELEASE_NAME}")
    ENDIF()

    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:${WGPU_LIB_DEBUG}>
        $<$<CONFIG:Release>:${WGPU_LIB_RELEASE}>)

    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME} PRIVATE
        dxgi
        d3d12)
ELSEIF(APPLE)
    IF(ARCHITECTURE MATCHES "^(x86_64|amd64)$")
        SET(WGPU_PLATFORM_DIR "${WGPU_DIR}/macos/x86_64")
        SET(WGPU_DEBUG_NAME "libwgpu_native_macos_x86_64_debug.a")
        SET(WGPU_RELEASE_NAME "libwgpu_native_macos_x86_64_release.a")
    ELSE()
        SET(WGPU_PLATFORM_DIR "${WGPU_DIR}/macos/aarch64")
        SET(WGPU_DEBUG_NAME "libwgpu_native_macos_aarch64_debug.a")
        SET(WGPU_RELEASE_NAME "libwgpu_native_macos_aarch64_release.a")
    ENDIF()

    SET(WGPU_LIB_DEBUG "${WGPU_PLATFORM_DIR}/debug/${WGPU_DEBUG_NAME}")
    SET(WGPU_LIB_RELEASE "${WGPU_PLATFORM_DIR}/release/${WGPU_RELEASE_NAME}")

    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:${WGPU_LIB_DEBUG}>
        $<$<CONFIG:Release>:${WGPU_LIB_RELEASE}>)

    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME} PRIVATE
        "-framework Metal"
        "-framework IOKit"
        "-framework Foundation")
ELSE()
    IF(ARCHITECTURE MATCHES "^(x86_64|amd64)$")
        SET(WGPU_PLATFORM_DIR "${WGPU_DIR}/linux/x86_64")
        SET(WGPU_DEBUG_NAME "libwgpu_native_linux_x86_64_debug.a")
        SET(WGPU_RELEASE_NAME "libwgpu_native_linux_x86_64_release.a")
    ELSE()
        SET(WGPU_PLATFORM_DIR "${WGPU_DIR}/linux/aarch64")
        SET(WGPU_DEBUG_NAME "libwgpu_native_linux_aarch64_debug.a")
        SET(WGPU_RELEASE_NAME "libwgpu_native_linux_aarch64_release.a")
    ENDIF()

    SET(WGPU_LIB_DEBUG "${WGPU_PLATFORM_DIR}/debug/${WGPU_DEBUG_NAME}")
    SET(WGPU_LIB_RELEASE "${WGPU_PLATFORM_DIR}/release/${WGPU_RELEASE_NAME}")

    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:${WGPU_LIB_DEBUG}>
        $<$<CONFIG:Release>:${WGPU_LIB_RELEASE}>)

    FIND_PACKAGE(Vulkan REQUIRED)
    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan)
ENDIF()

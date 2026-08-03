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
    SET(WINDOWS_WGPU_PATH "${WGPU_DIR}/windows/x86_64")

    IF(MSVC)
        SET(WGPU_LIB_DEBUG
            "${WINDOWS_WGPU_PATH}/msvc-debug/wgpu_native.lib")
        SET(WGPU_LIB_RELEASE
            "${WINDOWS_WGPU_PATH}/msvc-release/wgpu_native.lib")
    ELSE()
        SET(WGPU_LIB_DEBUG
            "${WINDOWS_WGPU_PATH}/gnu-debug/libwgpu_native.a")
        SET(WGPU_LIB_RELEASE
            "${WINDOWS_WGPU_PATH}/gnu-release/libwgpu_native.a")
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
        SET(APPLE_WGPU_PATH "${WGPU_DIR}/apple/x86_64")
    ELSE()
        SET(APPLE_WGPU_PATH "${WGPU_DIR}/apple/aarch64")
    ENDIF()

    SET(WGPU_LIB_DEBUG "${APPLE_WGPU_PATH}/debug/libwgpu_native.a")
    SET(WGPU_LIB_RELEASE "${APPLE_WGPU_PATH}/release/libwgpu_native.a")

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
        SET(LINUX_WGPU_PATH "${WGPU_DIR}/linux/x86_64")
    ELSE()
        SET(LINUX_WGPU_PATH "${WGPU_DIR}/linux/aarch64")
    ENDIF()

    SET(WGPU_LIB_DEBUG "${LINUX_WGPU_PATH}/debug/libwgpu_native.a")
    SET(WGPU_LIB_RELEASE "${LINUX_WGPU_PATH}/release/libwgpu_native.a")

    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:${WGPU_LIB_DEBUG}>
        $<$<CONFIG:Release>:${WGPU_LIB_RELEASE}>)

    FIND_PACKAGE(Vulkan REQUIRED)
    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan)
ENDIF()

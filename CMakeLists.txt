cmake_minimum_required(VERSION 3.21)
project(aria_tutorial VERSION 0.1.2 LANGUAGES CXX)

set(ARIA_DEPENDENCIES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.json" CACHE FILEPATH
    "Shared dependency requests and resolved versions")
set(ARIA_DEP_ARIA_VERSION "$ENV{ARIA_DEP_ARIA_VERSION}" CACHE STRING
    "Exact installed Aria SDK version (explicit ARIA_ROOT still takes precedence)")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 所有可执行文件和 Aria 动态库统一输出到 bin/，双击即可运行
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(MSVC)
  add_compile_options(/utf-8 /EHsc)
endif()

# ---------------------------------------------------------------------------
# 可选章节开关：第 15 章分别展示 Qt6 和 HTTP 适配器
# ---------------------------------------------------------------------------
option(ARIA_TUTORIAL_QT6  "构建第 15 章：Qt6 适配器示例（需 Qt6）"       OFF)
option(ARIA_TUTORIAL_HTTP "构建第 15 章：HTTP 适配器示例（需 HTTP 模块）" OFF)

# ---------------------------------------------------------------------------
# 定位 Aria：显式 ARIA_ROOT 优先，否则使用已安装的 3.x SDK
# ---------------------------------------------------------------------------
set(ARIA_ROOT "" CACHE PATH "Aria 3.x 源码树路径（优先于已安装 SDK）")
if(ARIA_ROOT)
  if(NOT EXISTS "${ARIA_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "ARIA_ROOT 不是 Aria 源码树：${ARIA_ROOT}")
  endif()
  message(STATUS "Aria: 使用源码树 ${ARIA_ROOT}")

  set(ARIA_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
  set(ARIA_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
  set(ARIA_BUILD_DOCS      OFF CACHE BOOL "" FORCE)
  set(ARIA_BUILD_QT6  ${ARIA_TUTORIAL_QT6}  CACHE BOOL "" FORCE)
  set(ARIA_BUILD_HTTP ${ARIA_TUTORIAL_HTTP} CACHE BOOL "" FORCE)

  add_subdirectory("${ARIA_ROOT}" "${CMAKE_BINARY_DIR}/_aria" EXCLUDE_FROM_ALL)
else()
  set(_aria_components core async runtime binding)
  if(ARIA_TUTORIAL_QT6)
    list(APPEND _aria_components qt6)
  endif()
  if(ARIA_TUTORIAL_HTTP)
    list(APPEND _aria_components http)
  endif()
  # Respect an explicit SDK prefix/version. Otherwise choose the newest
  # compatible SDK in the configured package search paths.
  set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
  set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
  if(ARIA_DEP_ARIA_VERSION)
    string(REGEX REPLACE "^v" "" _aria_sdk_version "${ARIA_DEP_ARIA_VERSION}")
    find_package(aria ${_aria_sdk_version} EXACT REQUIRED CONFIG COMPONENTS ${_aria_components})
  else()
    find_package(aria 3.0 REQUIRED CONFIG COMPONENTS ${_aria_components})
  endif()
  message(STATUS "Aria: 使用已安装的 3.x SDK")
endif()

include(CTest)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/TutorialRuntime.cmake")
add_subdirectory(demos)

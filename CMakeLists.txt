cmake_minimum_required(VERSION 3.20)
project(aria_tutorial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 所有可执行文件和 Aria 动态库统一输出到 bin/，双击即可运行
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(MSVC)
  add_compile_options(/utf-8 /EHsc)
endif()

# ---------------------------------------------------------------------------
# 可选章节开关：第 15 章需要 Qt6，第 16 章需要 Aria 的 HTTP 适配器
# ---------------------------------------------------------------------------
option(ARIA_TUTORIAL_QT6  "构建第 15 章：Qt6 适配器示例（需 Qt6）"       OFF)
option(ARIA_TUTORIAL_HTTP "构建第 16 章：HTTP 适配器示例（需 HTTP 模块）" OFF)

# ---------------------------------------------------------------------------
# 定位 Aria：优先用已安装的 SDK，其次用 ARIA_ROOT 指向的源码树
# ---------------------------------------------------------------------------
find_package(aria 2.0 QUIET CONFIG)

if(aria_FOUND)
  message(STATUS "Aria: 使用已安装的 SDK")
else()
  set(ARIA_ROOT "" CACHE PATH "Aria 源码树路径（未安装 SDK 时使用）")
  if(NOT ARIA_ROOT)
    message(FATAL_ERROR
      "未找到 Aria。二选一：\n"
      "  1) 先安装 Aria SDK，再重新配置\n"
      "  2) 配置时指定源码树：cmake -S . -B build -DARIA_ROOT=<path-to-Aria>")
  endif()
  message(STATUS "Aria: 使用源码树 ${ARIA_ROOT}")

  set(ARIA_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
  set(ARIA_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
  if(ARIA_TUTORIAL_QT6)
    set(ARIA_BUILD_QT6  ON  CACHE BOOL "" FORCE)
  endif()
  if(ARIA_TUTORIAL_HTTP)
    set(ARIA_BUILD_HTTP ON  CACHE BOOL "" FORCE)
  endif()

  add_subdirectory("${ARIA_ROOT}" "${CMAKE_BINARY_DIR}/_aria" EXCLUDE_FROM_ALL)
endif()

add_subdirectory(demos)

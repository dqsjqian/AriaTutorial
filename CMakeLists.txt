cmake_minimum_required(VERSION 3.21)
project(aria_tutorial VERSION 0.2.0 LANGUAGES CXX)

set(ARIA_DEPENDENCIES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.json" CACHE FILEPATH
    "Shared dependency requests and resolved versions")
set(ARIA_DEP_ARIA_VERSION "$ENV{ARIA_DEP_ARIA_VERSION}" CACHE STRING
    "Exact installed Aria SDK version (explicit ARIA_ROOT still takes precedence)")
set(ARIA_SDK_PREFIX "" CACHE PATH "Restrict installed Aria SDK lookup to this prefix")

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
option(ARIA_TUTORIAL_HTTP_TLS "Enable TLS in the selected HTTP SDK" OFF)
if(ARIA_TUTORIAL_HTTP_TLS AND NOT ARIA_TUTORIAL_HTTP)
    message(FATAL_ERROR "ARIA_TUTORIAL_HTTP_TLS requires ARIA_TUTORIAL_HTTP")
endif()

# Default and editable-source SDKs both live in the shared ForgeDeps store.
set(ARIA_ROOT "" CACHE PATH "Optional editable Aria source for the shared SDK recipe")
set(ARIA_SDK_SOURCE "${ARIA_ROOT}")
set(ARIA_SDK_QT ${ARIA_TUTORIAL_QT6})
set(ARIA_SDK_HTTP ${ARIA_TUTORIAL_HTTP})
set(ARIA_SDK_TLS ${ARIA_TUTORIAL_HTTP_TLS})
set(ARIA_SDK_SHARED OFF)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SharedAria.cmake")

include(CTest)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/TutorialRuntime.cmake")
add_subdirectory(demos)

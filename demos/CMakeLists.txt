# 每个 demo 对应教程的一章，只依赖 Aria 的公开库（纯 C++，无 UI 依赖）
#
# aria_add_demo(<name> [额外链接库...])
# 头文件搜索路径包含 demos/ 本身, 方便共用 common/ 下的辅助代码。

function(aria_add_demo name)
  add_executable(${name} ${name}/main.cpp)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${name} PRIVATE aria::core aria::async aria::runtime ${ARGN})
endfunction()

# ---- 第一阶段：入门 ----
aria_add_demo(ch01_bill)
aria_add_demo(ch02_hello)
aria_add_demo(ch03_property)

# ---- 第二阶段：响应式核心 ----
aria_add_demo(ch04_computed)
aria_add_demo(ch05_batch)
aria_add_demo(ch06_subscription)
aria_add_demo(ch07_pitfalls)
aria_add_demo(ch08_command)

# ---- 第三阶段：集合与表单 ----
aria_add_demo(ch09_list)
aria_add_demo(ch10_derived)
aria_add_demo(ch11_reconcile)
aria_add_demo(ch12_validation)

# ---- 第四阶段：架构与适配 ----
aria_add_demo(ch13_viewmodel aria::binding)
aria_add_demo(ch14_binding aria::binding)

# 第 15 章有 Qt6 与 HTTP 两个示例, 需要额外的框架模块, 默认跳过。
if(TARGET aria::qt6)
  aria_add_demo(ch15_qt6 aria::qt6)
  message(STATUS "AriaTutorial: 第 15 章 Qt6 示例已启用")
else()
  message(STATUS "AriaTutorial: 跳过 ch15_qt6 -- 需要 -DARIA_TUTORIAL_QT6=ON 且本机有 Qt6")
endif()

if(TARGET aria::http)
  aria_add_demo(ch15_http aria::http)
  message(STATUS "AriaTutorial: 第 15 章 HTTP 示例已启用")
else()
  message(STATUS "AriaTutorial: 跳过 ch15_http -- 需要 -DARIA_TUTORIAL_HTTP=ON")
endif()

aria_add_demo(ch16_adapter aria::binding)

# ---- 第五阶段：精通 ----
aria_add_demo(ch17_async)
aria_add_demo(ch18_diagnostics)
aria_add_demo(ch19_testing)
aria_add_demo(ch20_ecosystem aria::binding)

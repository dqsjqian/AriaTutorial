# Deploy the shared Windows runtime once per build, before any demo is linked.
# A single target avoids concurrent demos copying the same DLL into bin/Release.
function(aria_tutorial_deploy_windows_runtime)
  if(NOT WIN32)
    return()
  endif()

  set(runtime_libraries "")
  set(runtime_search_dirs "")
  set(platform_plugins "")
  foreach(runtime_target aria::abi aria::runtime aria::binding aria::qt6 aria::http
                         Qt6::Core Qt6::Gui Qt6::Widgets)
    if(TARGET ${runtime_target})
      get_target_property(runtime_type ${runtime_target} TYPE)
      if(runtime_type STREQUAL "SHARED_LIBRARY")
        list(APPEND runtime_libraries "$<TARGET_FILE:${runtime_target}>")
        list(APPEND runtime_search_dirs "$<TARGET_FILE_DIR:${runtime_target}>")
      endif()
    endif()
  endforeach()

  if(ARIA_TUTORIAL_QT6)
    set(required_plugins Qt6::QWindowsIntegrationPlugin)
    if(BUILD_TESTING)
      list(APPEND required_plugins Qt6::QOffscreenIntegrationPlugin)
    endif()
    foreach(plugin IN LISTS required_plugins)
      if(NOT TARGET ${plugin})
        message(FATAL_ERROR "The selected Qt kit does not provide ${plugin}")
      endif()
      get_target_property(plugin_type ${plugin} TYPE)
      if(plugin_type STREQUAL "STATIC_LIBRARY")
        qt6_import_plugins(ch15_qt6 INCLUDE ${plugin})
      else()
        list(APPEND platform_plugins "$<TARGET_FILE:${plugin}>")
      endif()
    endforeach()
  endif()

  get_filename_component(compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
  list(APPEND runtime_search_dirs "${compiler_dir}")
  if(MINGW)
    # With a static Aria build there are no framework DLLs to reveal these
    # dependencies, so ask the selected compiler for its own runtime files.
    foreach(dll libstdc++-6.dll libwinpthread-1.dll libgcc_s_seh-1.dll
                libgcc_s_dw2-1.dll libgcc_s_sjlj-1.dll)
      execute_process(COMMAND "${CMAKE_CXX_COMPILER}" "-print-file-name=${dll}"
        OUTPUT_VARIABLE dll_path OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE query_status)
      if(query_status EQUAL 0 AND IS_ABSOLUTE "${dll_path}" AND EXISTS "${dll_path}")
        list(APPEND runtime_libraries "${dll_path}")
        get_filename_component(dll_dir "${dll_path}" DIRECTORY)
        list(APPEND runtime_search_dirs "${dll_dir}")
      endif()
    endforeach()
  endif()

  list(GET ARGN 0 first_demo)
  set(manifest "${CMAKE_CURRENT_BINARY_DIR}/runtime-$<CONFIG>.cmake")
  set(runtime_tool "")
  if(MINGW AND CMAKE_OBJDUMP)
    set(runtime_tool "set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL objdump)\nset(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND [==[${CMAKE_OBJDUMP}]==])\n")
  endif()
  file(GENERATE OUTPUT "${manifest}" CONTENT
"set(runtime_libraries [==[${runtime_libraries}]==])
set(platform_plugins [==[${platform_plugins}]==])
set(runtime_search_dirs [==[${runtime_search_dirs}]==])
set(runtime_output [==[$<TARGET_FILE_DIR:${first_demo}>]==])
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM windows+pe)
${runtime_tool}include([==[${CMAKE_CURRENT_FUNCTION_LIST_DIR}/DeployTutorialRuntime.cmake]==])
")
  add_custom_target(aria_tutorial_runtime
    COMMAND "${CMAKE_COMMAND}" -P "${manifest}"
    DEPENDS ${runtime_libraries} ${platform_plugins}
    COMMENT "Deploying Aria and Qt runtime beside the tutorial demos"
    VERBATIM)
  foreach(demo IN LISTS ARGN)
    add_dependencies(${demo} aria_tutorial_runtime)
  endforeach()
endfunction()

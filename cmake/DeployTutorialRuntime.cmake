cmake_minimum_required(VERSION 3.20)

if(NOT runtime_output)
  message(FATAL_ERROR "runtime_output is required")
endif()
file(MAKE_DIRECTORY "${runtime_output}")
# Also serialize independent build invocations sharing this output directory.
file(LOCK "${runtime_output}/.aria-runtime.lock" GUARD PROCESS TIMEOUT 120)

list(REMOVE_DUPLICATES runtime_libraries)
list(REMOVE_DUPLICATES platform_plugins)
list(REMOVE_DUPLICATES runtime_search_dirs)
# Exclude only the actual OS installation, not SDK directories named Windows.
set(system_directories "")
foreach(system_root "$ENV{SystemRoot}" "$ENV{WINDIR}")
  if(system_root STREQUAL "")
    continue()
  endif()
  file(TO_CMAKE_PATH "${system_root}" system_root)
  string(REGEX REPLACE "/+$" "" system_root "${system_root}")
  string(LENGTH "${system_root}" root_length)
  math(EXPR last_character "${root_length} - 1")
  set(system_pattern "^")
  foreach(character_index RANGE ${last_character})
    string(SUBSTRING "${system_root}" ${character_index} 1 character)
    if(character MATCHES "[A-Za-z]")
      string(TOLOWER "${character}" lower)
      string(TOUPPER "${character}" upper)
      string(APPEND system_pattern "[${lower}${upper}]")
    else()
      # Escape regex metacharacters in a custom Windows installation path.
      string(REGEX REPLACE "([][+.*()^$?|\\\\])" "\\\\\\1" character "${character}")
      string(APPEND system_pattern "${character}")
    endif()
  endforeach()
  list(APPEND system_directories "${system_pattern}[/\\\\]")
endforeach()
if(runtime_libraries OR platform_plugins)
  # Scan original SDK files, not yesterday's deployed copies. This refreshes
  # transitive DLLs when an installed SDK changes at the same path.
  file(GET_RUNTIME_DEPENDENCIES
    LIBRARIES ${runtime_libraries}
    MODULES ${platform_plugins}
    DIRECTORIES ${runtime_search_dirs}
    RESOLVED_DEPENDENCIES_VAR runtime_dependencies
    UNRESOLVED_DEPENDENCIES_VAR missing_dependencies
    CONFLICTING_DEPENDENCIES_PREFIX conflicts
    PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
    POST_EXCLUDE_REGEXES ${system_directories})
  if(missing_dependencies)
    message(FATAL_ERROR "Tutorial runtime DLLs could not be resolved: ${missing_dependencies}")
  endif()
  if(conflicts_FILENAMES)
    message(FATAL_ERROR "Conflicting tutorial runtime DLLs: ${conflicts_FILENAMES}")
  endif()
  list(APPEND runtime_libraries ${runtime_dependencies})
  list(REMOVE_DUPLICATES runtime_libraries)
endif()

function(copy_runtime_file source destination_dir)
  get_filename_component(name "${source}" NAME)
  get_filename_component(source_absolute "${source}" ABSOLUTE)
  get_filename_component(destination_absolute "${destination_dir}/${name}" ABSOLUTE)
  if(NOT source_absolute STREQUAL destination_absolute)
    file(MAKE_DIRECTORY "${destination_dir}")
    configure_file("${source}" "${destination_dir}/${name}" COPYONLY)
  endif()
endfunction()

foreach(library IN LISTS runtime_libraries)
  copy_runtime_file("${library}" "${runtime_output}")
endforeach()
foreach(plugin IN LISTS platform_plugins)
  copy_runtime_file("${plugin}" "${runtime_output}/platforms")
endforeach()

# App policy: build one shared Aria SDK, including explicit development sources.
include_guard(GLOBAL)
if(NOT FORGEDEPS_PYTHON_EXECUTABLE)
    find_package(Python3 3.14 REQUIRED COMPONENTS Interpreter)
    set(FORGEDEPS_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}" CACHE FILEPATH "ForgeDeps interpreter")
endif()
set(ForgeDeps_PYTHON_EXECUTABLE "${FORGEDEPS_PYTHON_EXECUTABLE}")
execute_process(COMMAND "${FORGEDEPS_PYTHON_EXECUTABLE}" -c
    "from pathlib import Path; import forgerecipes; print(Path(forgerecipes.__file__).parent / 'cmake' / 'AriaSdk.cmake')"
    RESULT_VARIABLE _sdk_status OUTPUT_VARIABLE _sdk_module ERROR_VARIABLE _sdk_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT _sdk_status EQUAL 0)
    message(FATAL_ERROR "Install requirements-build.txt with ${FORGEDEPS_PYTHON_EXECUTABLE}: ${_sdk_error}")
endif()
include("${_sdk_module}")
if(NOT ARIA_SDK_SOURCE AND DEFINED ENV{ARIA_SDK_SOURCE})
    set(ARIA_SDK_SOURCE "$ENV{ARIA_SDK_SOURCE}")
endif()
if(NOT ARIA_SDK_PREFIX)
    foreach(_feature QT HTTP TLS JNI APPKIT UIKIT SHARED)
        if(NOT DEFINED ARIA_SDK_${_feature})
            set(ARIA_SDK_${_feature} OFF)
        endif()
    endforeach()
    set(_source_args)
    if(ARIA_SDK_SOURCE)
        list(APPEND _source_args SOURCE "${ARIA_SDK_SOURCE}")
    endif()
    forgerecipes_sdk(FILE "${ARIA_DEPENDENCIES_FILE}" ${_source_args}
        QT "${ARIA_SDK_QT}" HTTP "${ARIA_SDK_HTTP}" TLS "${ARIA_SDK_TLS}"
        JNI "${ARIA_SDK_JNI}" APPKIT "${ARIA_SDK_APPKIT}" UIKIT "${ARIA_SDK_UIKIT}"
        SHARED "${ARIA_SDK_SHARED}" PREFIXES ARIA_SDK_PREFIXES)
    # The recipe package layout is owned by the recipe layer, not duplicated here.
    # The helper also sets the aria_DIR cache entry for the lookup below.
    forgerecipes_locate_package(PREFIXES ${ARIA_SDK_PREFIXES} PACKAGE aria
        OUT _aria_config_dir PREFIX_OUT ARIA_SDK_PREFIX)
    if(NOT ARIA_SDK_PREFIX)
        message(FATAL_ERROR "The shared environment did not install an Aria SDK")
    endif()
endif()
set(_aria_components core async runtime binding)
foreach(_feature QT HTTP JNI APPKIT UIKIT)
    if(ARIA_SDK_${_feature})
        string(TOLOWER "${_feature}" _component)
        if(_feature STREQUAL "QT")
            set(_component qt6)
        endif()
        list(APPEND _aria_components "${_component}")
    endif()
endforeach()
find_package(aria ${ARIA_DEP_ARIA_VERSION} EXACT CONFIG REQUIRED COMPONENTS ${_aria_components}
    PATHS "${ARIA_SDK_PREFIX}" NO_DEFAULT_PATH)

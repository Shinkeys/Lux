# Script to install slang. As it is incovenient to install slang via submodules, because you would need to 
# install every dependency

include(FetchContent)

function(download_slang)
    set(SLANG_TARGET_DIR "${CMAKE_SOURCE_DIR}/vendor/slang")
    set(SLANG_BIN "${SLANG_TARGET_DIR}/bin")

    if(EXISTS "${SLANG_BIN}/slangc" OR EXISTS "${SLANG_BIN}/slang.dll")
        message(STATUS "Slang already installed in ${SLANG_TARGET_DIR}")
        setup_slang_variables(${SLANG_TARGET_DIR})
        return()
    endif()

    set(TEMP_DIR "${CMAKE_BINARY_DIR}/slang_tmp")
    file(MAKE_DIRECTORY "${TEMP_DIR}")

    set(RELEASE_API "https://api.github.com/repos/shader-slang/slang/releases/latest")
    file(DOWNLOAD "${RELEASE_API}" "${TEMP_DIR}/release.json" STATUS STATUS_LIST)
    list(GET STATUS_LIST 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to fetch Slang release info")
    endif()

    file(READ "${TEMP_DIR}/release.json" RELEASE_JSON)
    string(REGEX MATCH "\"tag_name\"[ \t]*:[ \t]*\"([^\"]+)\"" _ "${RELEASE_JSON}")
    set(VERSION "${CMAKE_MATCH_1}")
    string(REGEX REPLACE "^v" "" VERSION_NUM "${VERSION}")

    if(WIN32)
        set(ARCHIVE "slang-2025.12.1-windows-x86_64.zip")
    elseif(UNIX AND NOT APPLE)
        set(ARCHIVE "slang-2025.12.1-linux-x86_64.zip")
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()

    set(URL "https://github.com/shader-slang/slang/releases/download/v2025.12.1/${ARCHIVE}")
    set(ZIP_PATH "${TEMP_DIR}/${ARCHIVE}")

    message(STATUS "Downloading Slang from ${URL}")
    file(DOWNLOAD "${URL}" "${ZIP_PATH}" SHOW_PROGRESS STATUS STATUS_LIST)
    list(GET STATUS_LIST 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download Slang archive")
    endif()

    file(MAKE_DIRECTORY "${SLANG_TARGET_DIR}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${ZIP_PATH}" WORKING_DIRECTORY "${SLANG_TARGET_DIR}" RESULT_VARIABLE RES)
    if(NOT RES EQUAL 0)
        # fallback for Windows zip
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "${ZIP_PATH}" WORKING_DIRECTORY "${SLANG_TARGET_DIR}" RESULT_VARIABLE RES_ZIP)
        if(NOT RES_ZIP EQUAL 0)
            message(FATAL_ERROR "Failed to extract Slang archive")
        endif()
    endif()

    # Flatten one extra subdir if needed
    file(GLOB SLANG_SUBDIR "${SLANG_TARGET_DIR}/slang-*")
    list(LENGTH SLANG_SUBDIR COUNT)
    if(COUNT EQUAL 1)
        list(GET SLANG_SUBDIR 0 ACTUAL_DIR)
        file(GLOB CONTENTS "${ACTUAL_DIR}/*")
        foreach(ITEM ${CONTENTS})
            get_filename_component(NAME ${ITEM} NAME)
            file(RENAME "${ITEM}" "${SLANG_TARGET_DIR}/${NAME}")
        endforeach()
        file(REMOVE_RECURSE "${ACTUAL_DIR}")
    endif()

    file(REMOVE_RECURSE "${TEMP_DIR}")
    setup_slang_variables(${SLANG_TARGET_DIR})
endfunction()

function(setup_slang_variables ROOT)
    set(SLANG_INCLUDE_DIR "${ROOT}/include" CACHE PATH "")
    set(SLANG_LIBRARY_DIR "${ROOT}/lib" CACHE PATH "")
    set(SLANG_BINARY_DIR "${ROOT}/bin" CACHE PATH "")

    set(SLANG_INCLUDE_DIR "${SLANG_INCLUDE_DIR}" PARENT_SCOPE)
    set(SLANG_LIBRARY_DIR "${SLANG_LIBRARY_DIR}" PARENT_SCOPE)
    set(SLANG_BINARY_DIR "${SLANG_BINARY_DIR}" PARENT_SCOPE)

    message(STATUS "Slang paths:")
    message(STATUS "  Include: ${SLANG_INCLUDE_DIR}")
    message(STATUS "  Lib:     ${SLANG_LIBRARY_DIR}")
    message(STATUS "  Bin:     ${SLANG_BINARY_DIR}")
endfunction()

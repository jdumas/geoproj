################################################################################
cmake_minimum_required(VERSION 3.11)

set(GAZEBO_EXTERNAL "${CMAKE_CURRENT_LIST_DIR}/../external")

set(GAZEBO_CACHE_DIR ".cache_${CMAKE_GENERATOR}")
string(REGEX REPLACE "[^A-Za-z0-9_.]" "_" GAZEBO_CACHE_DIR ${GAZEBO_CACHE_DIR})
string(TOLOWER ${GAZEBO_CACHE_DIR} GAZEBO_CACHE_DIR)

function(gazebo_declare name)
    include(FetchContent)
    FetchContent_Declare(
        ${name}
        SOURCE_DIR   ${GAZEBO_EXTERNAL}/${name}
        DOWNLOAD_DIR ${GAZEBO_EXTERNAL}/${GAZEBO_CACHE_DIR}/${name}
        SUBBUILD_DIR ${GAZEBO_EXTERNAL}/${GAZEBO_CACHE_DIR}/build/${name}
        QUIET
        TLS_VERIFY OFF
        GIT_CONFIG advice.detachedHead=false
        ${ARGN}
    )
endfunction()

################################################################################
# Generic fetch behavior
################################################################################

# Default fetch function (fetch only)
function(gazebo_fetch)
    include(FetchContent)
    foreach(name IN ITEMS ${ARGN})
        FetchContent_GetProperties(${name})
        if(NOT ${name}_POPULATED)
            FetchContent_Populate(${name})
        endif()
    endforeach()
endfunction()

################################################################################
# Generic import behavior
################################################################################

# Default import function (fetch + add_subdirectory)
function(gazebo_import_default name)
    include(FetchContent)
    FetchContent_GetProperties(${name})
    if(NOT ${name}_POPULATED)
        FetchContent_Populate(${name})
        add_subdirectory(${${name}_SOURCE_DIR} ${${name}_BINARY_DIR})
    endif()
endfunction()

file(WRITE ${CMAKE_BINARY_DIR}/GazeboImport.cmake.in
    "if(COMMAND gazebo_import_@NAME@)\n\tgazebo_import_@NAME@()\nelse()\n\tgazebo_import_default(@NAME@)\nendif()"
)

# Use some meta-programming to call gazebo_import_foo if such a function is user-defined,
# otherwise, we defer to the default behavior which is to call gazebo_import_default(foo)
set_property(GLOBAL PROPERTY __GAZEBO_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
function(gazebo_import)
    get_property(GAZEBO_CMAKE_DIR GLOBAL PROPERTY __GAZEBO_CMAKE_DIR)
    foreach(NAME IN ITEMS ${ARGN})
        set(__import_file "${CMAKE_BINARY_DIR}/gazebo_import_${NAME}.cmake")
        configure_file("${CMAKE_BINARY_DIR}/GazeboImport.cmake.in" "${__import_file}" @ONLY)
        include("${__import_file}")
    endforeach()
endfunction()

################################################################################
# Customized import functions
################################################################################

function(gazebo_import_tbb)
    if(NOT TARGET tbb::tbb)
        # Download tbb
        gazebo_fetch(tbb)
        FetchContent_GetProperties(tbb)

        # Create tbb:tbb target
        set(TBB_BUILD_STATIC ON CACHE BOOL " " FORCE)
        set(TBB_BUILD_SHARED OFF CACHE BOOL " " FORCE)
        set(TBB_BUILD_TBBMALLOC OFF CACHE BOOL " " FORCE)
        set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL " " FORCE)
        set(TBB_BUILD_TESTS OFF CACHE BOOL " " FORCE)
        set(TBB_NO_DATE ON CACHE BOOL " " FORCE)

        add_subdirectory(${tbb_SOURCE_DIR} ${tbb_BINARY_DIR})
        set_target_properties(tbb_static PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${tbb_SOURCE_DIR}/include"
        )
        if(NOT MSVC)
            set_target_properties(tbb_static PROPERTIES
                COMPILE_FLAGS "-Wno-implicit-fallthrough -Wno-missing-field-initializers -Wno-unused-parameter -Wno-keyword-macro"
            )
            set_target_properties(tbb_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
        endif()
        add_library(tbb::tbb ALIAS tbb_static)
    endif()
endfunction()

function(gazebo_import_geogram)
    if(NOT TARGET geogram::geogram)
        # Download Geogram
        gazebo_fetch(geogram)
        FetchContent_GetProperties(geogram)

        # Create Geogram target
        include(geogram)
    endif()
endfunction()

function(gazebo_import_sanitizers)
    gazebo_fetch(sanitizers)
    FetchContent_GetProperties(sanitizers)
    list(APPEND CMAKE_MODULE_PATH "${sanitizers_SOURCE_DIR}/cmake")
    list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
    find_package(Sanitizers REQUIRED)
endfunction()

################################################################################
# Declare third-party dependencies here
################################################################################

gazebo_declare(tbb
    GIT_REPOSITORY https://github.com/wjakob/tbb.git
    GIT_TAG        b066defc0229a1e92d7a200eb3fe0f7e35945d95
)

gazebo_declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v1.8.0
)

gazebo_declare(geogram
    GIT_REPOSITORY https://github.com/jdumas/geogram.git
    GIT_TAG        592cad213c06cfcb1197ddb84b7e7a64a97e620c
)

gazebo_declare(sanitizers
    GIT_REPOSITORY https://github.com/arsenm/sanitizers-cmake.git
    GIT_TAG        99e159ec9bc8dd362b08d18436bd40ff0648417b
)

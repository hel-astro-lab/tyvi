cmake_minimum_required(VERSION 3.21)

# This template attempts to be "fetch_content"-able
# so that it works well with tools like CPM or other
# manual dependency management

# Only set the cxx_standard if it is not set by someone else
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled (note that PCH support is removed).
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the project name and language
project(
    tyvi
    VERSION 0.0.1
    DESCRIPTION ""
    HOMEPAGE_URL "https://github.com/hel-astro-lab"
    LANGUAGES CXX
)

include(cmake/PreventInSourceBuilds.cmake)
include(ProjectOptions.cmake)

tyvi_declare_options()
tyvi_setup_global_options()
tyvi_setup_local_options()

if(PROJECT_IS_TOP_LEVEL)
    # Declares ENABLE_TESTING option (default: ON).
    # Needs to happen before dependencies, because we fetch boost-ext/ut based on it.
    include(CTest)
endif()

include(Dependencies.cmake)
tyvi_setup_dependencies()

# Potentially useful, see: https://gcc.gnu.org/wiki/Visibility
# set(CMAKE_CXX_VISIBILITY_PRESET hidden)

target_compile_features(tyvi_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(tyvi::tyvi_options ALIAS tyvi_options)
add_library(tyvi::tyvi_warnings ALIAS tyvi_warnings)

# Adding the src:
add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
    message(STATUS "Adding tyvi tests.")
    add_subdirectory(test)
else()
    message(STATUS "Skipping tyvi tests.")
endif()

if(CMAKE_SKIP_INSTALL_RULES)
    return()
endif()

# Setup project installation here (if neccesseary).

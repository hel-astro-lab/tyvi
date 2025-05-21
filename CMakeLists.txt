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
# when compiling with PCH enabled
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

myproject_setup_options()

myproject_global_options()
include(Dependencies.cmake)
myproject_setup_dependencies()

myproject_local_options()

# Potentially useful, see: https://gcc.gnu.org/wiki/Visibility
# set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(GIT_SHA
    "Unknown"
    CACHE STRING "SHA this build was generated from"
)
string(
    SUBSTRING "${GIT_SHA}"
              0
              8
              GIT_SHORT_SHA
)

target_compile_features(myproject_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(myproject::myproject_options ALIAS myproject_options)
add_library(myproject::myproject_warnings ALIAS myproject_warnings)

# Adding the src:
add_subdirectory(src)

# Don't even look at tests if we're not top level
if(NOT PROJECT_IS_TOP_LEVEL)
    return()
endif()

# Adding the tests:
include(CTest)

if(BUILD_TESTING)
    message(AUTHOR_WARNING "Building Tests.")
    add_subdirectory(test)
endif()

if(CMAKE_SKIP_INSTALL_RULES)
    return()
endif()

# Setup project installation here (if neccesseary).

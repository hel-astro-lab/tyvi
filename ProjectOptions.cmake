include(CMakeDependentOption)
include(CheckCXXCompilerFlag)

include(CheckCXXSourceCompiles)

# Checks if configuration supports undefined and address sanitizer.
# Results are stored in variables:
#
# SUPPORTS_UBSAN = <ON|OFF>
# SUPPORTS_ASAN = <ON|OFF>
macro(MYPROJECT_CHECK_SANITIZER_SUPPORT)
    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*")
        message(STATUS [[
Sanity checking UndefinedBehaviorSanitizer, it should be supported on this platform.
]]
        )
        set(TEST_PROGRAM "int main() { return 0; }")

        # Check if UndefinedBehaviorSanitizer works at link time
        set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
        check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)
        set(CMAKE_REQUIRED_FLAGS "")
        set(CMAKE_REQUIRED_LINK_OPTIONS "")

        if(HAS_UBSAN_LINK_SUPPORT)
            message(STATUS "UndefinedBehaviorSanitizer is supported at both compile and link time.")
            set(SUPPORTS_UBSAN ON)
        else()
            message(WARNING "UndefinedBehaviorSanitizer is NOT supported at link time.")
            set(SUPPORTS_UBSAN OFF)
        endif()
    else()
        message(STATUS "current configuration doesn't support ubsan with: ${CMAKE_CXX_COMPILER_ID}")
        set(SUPPORTS_UBSAN OFF)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*")
        message(STATUS "Sanity checking AddressSanitizer, it should be supported on this platform")
        set(TEST_PROGRAM "int main() { return 0; }")

        # Check if AddressSanitizer works at link time
        set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
        check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)
        set(CMAKE_REQUIRED_FLAGS "")
        set(CMAKE_REQUIRED_LINK_OPTIONS "")

        if(HAS_ASAN_LINK_SUPPORT)
            message(STATUS "AddressSanitizer is supported at both compile and link time.")
            set(SUPPORTS_ASAN ON)
        else()
            message(WARNING "AddressSanitizer is NOT supported at link time.")
            set(SUPPORTS_ASAN OFF)
        endif()
    else()
        message(STATUS "current configuration doesn't support asan with: ${CMAKE_CXX_COMPILER_ID}")
        set(SUPPORTS_ASAN OFF)
    endif()
endmacro()

# Declares options for the project.
macro(MYPROJECT_DECLARE_OPTIONS)
    option(myproject_ENABLE_HARDENING "Enable hardening" ON)
    option(myproject_ENABLE_COVERAGE "Enable coverage reporting" OFF)
    cmake_dependent_option(
        myproject_ENABLE_GLOBAL_HARDENING
        "Attempt to push hardening options to built dependencies"
        ON
        myproject_ENABLE_HARDENING
        OFF
    )

    myproject_check_sanitizer_support()

    if(NOT PROJECT_IS_TOP_LEVEL OR myproject_PACKAGING_MAINTAINER_MODE)
        option(myproject_ENABLE_IPO "Enable IPO/LTO" OFF)
        option(myproject_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
        option(myproject_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
        option(myproject_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
        option(myproject_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
        option(myproject_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
        option(myproject_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
        option(myproject_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
        option(myproject_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
        option(myproject_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
        option(myproject_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
        option(myproject_ENABLE_CACHE "Enable ccache" OFF)
    else()
        option(myproject_ENABLE_IPO "Enable IPO/LTO" ON)
        option(myproject_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
        option(myproject_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
        option(myproject_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
        option(myproject_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
        option(myproject_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
        option(myproject_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
        option(myproject_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
        option(myproject_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
        option(myproject_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
        option(myproject_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
        option(myproject_ENABLE_CACHE "Enable ccache" OFF)
    endif()

    if(NOT PROJECT_IS_TOP_LEVEL)
        mark_as_advanced(
            myproject_ENABLE_IPO
            myproject_WARNINGS_AS_ERRORS
            myproject_ENABLE_USER_LINKER
            myproject_ENABLE_SANITIZER_ADDRESS
            myproject_ENABLE_SANITIZER_LEAK
            myproject_ENABLE_SANITIZER_UNDEFINED
            myproject_ENABLE_SANITIZER_THREAD
            myproject_ENABLE_SANITIZER_MEMORY
            myproject_ENABLE_UNITY_BUILD
            myproject_ENABLE_CLANG_TIDY
            myproject_ENABLE_CPPCHECK
            myproject_ENABLE_COVERAGE
            myproject_ENABLE_CACHE
        )
    endif()
endmacro()

# Setup options for the project and all the dependencies.
macro(MYPROJECT_SETUP_GLOBAL_OPTIONS)
    if(myproject_ENABLE_IPO)
        include(cmake/InterproceduralOptimization.cmake)
        myproject_configure_ipo()
    endif()

    myproject_check_sanitizer_support()

    if(myproject_ENABLE_HARDENING AND myproject_ENABLE_GLOBAL_HARDENING)
        include(cmake/Hardening.cmake)
        if(NOT SUPPORTS_UBSAN
           OR myproject_ENABLE_SANITIZER_UNDEFINED
           OR myproject_ENABLE_SANITIZER_ADDRESS
           OR myproject_ENABLE_SANITIZER_THREAD
           OR myproject_ENABLE_SANITIZER_LEAK
        )
            set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
        else()
            set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
        endif()
        message("enable hardening: ${myproject_ENABLE_HARDENING}")
        message("enable ubsan minimal runtime: ${ENABLE_UBSAN_MINIMAL_RUNTIME}")
        message("enable sanitizer undefined: ${myproject_ENABLE_SANITIZER_UNDEFINED}")
        myproject_setup_hardening(myproject_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
    endif()
endmacro()

# Setup options for the project.
macro(MYPROJECT_SETUP_LOCAL_OPTIONS)
    if(PROJECT_IS_TOP_LEVEL)
        include(cmake/StandardProjectSettings.cmake)
    endif()

    add_library(myproject_warnings INTERFACE)
    add_library(myproject_options INTERFACE)

    include(cmake/CompilerWarnings.cmake)
    myproject_setup_target_warnings(myproject_warnings ${myproject_WARNINGS_AS_ERRORS})

    if(myproject_ENABLE_USER_LINKER)
        include(cmake/Linker.cmake)
        myproject_configure_target_linker(myproject_options)
    endif()

    include(cmake/Sanitizers.cmake)
    myproject_setup_target_sanitizers(
        myproject_options
        ${myproject_ENABLE_SANITIZER_ADDRESS}
        ${myproject_ENABLE_SANITIZER_LEAK}
        ${myproject_ENABLE_SANITIZER_UNDEFINED}
        ${myproject_ENABLE_SANITIZER_THREAD}
        ${myproject_ENABLE_SANITIZER_MEMORY}
    )

    set_target_properties(myproject_options PROPERTIES UNITY_BUILD ${myproject_ENABLE_UNITY_BUILD})

    if(myproject_ENABLE_CACHE)
        include(cmake/Cache.cmake)
        myproject_enable_cache()
    endif()

    include(cmake/StaticAnalyzers.cmake)
    if(myproject_ENABLE_CLANG_TIDY)
        myproject_setup_clang_tidy(${myproject_WARNINGS_AS_ERRORS})
    endif()

    if(myproject_ENABLE_CPPCHECK)
        myproject_setup_cppcheck(${myproject_WARNINGS_AS_ERRORS} "")
    endif()

    if(myproject_ENABLE_COVERAGE)
        include(cmake/Tests.cmake)
        myproject_setup_target_coverage(myproject_options)
    endif()

    if(myproject_WARNINGS_AS_ERRORS)
        check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
        if(LINKER_FATAL_WARNINGS)
            # This is not working consistently, so disabling for now
            # target_link_options(myproject_options INTERFACE -Wl,--fatal-warnings)
        endif()
    endif()

    if(myproject_ENABLE_HARDENING AND NOT myproject_ENABLE_GLOBAL_HARDENING)
        include(cmake/Hardening.cmake)
        if(NOT SUPPORTS_UBSAN
           OR myproject_ENABLE_SANITIZER_UNDEFINED
           OR myproject_ENABLE_SANITIZER_ADDRESS
           OR myproject_ENABLE_SANITIZER_THREAD
           OR myproject_ENABLE_SANITIZER_LEAK
        )
            set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
        else()
            set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
        endif()
        myproject_setup_hardening(myproject_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
    endif()
endmacro()

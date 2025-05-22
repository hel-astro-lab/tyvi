include(CMakeDependentOption)
include(CheckCXXCompilerFlag)

include(CheckCXXSourceCompiles)

# Checks if configuration supports undefined and address sanitizer.
# Results are stored in variables:
#
# SUPPORTS_UBSAN = <ON|OFF>
# SUPPORTS_ASAN = <ON|OFF>
macro(TYVI_CHECK_SANITIZER_SUPPORT)
    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*")
        message(VERBOSE "Sanity checking ubsan, it should be supported on this platform.")
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
macro(TYVI_DECLARE_OPTIONS)
    option(tyvi_ENABLE_HARDENING "Enable hardening" ON)
    option(tyvi_ENABLE_COVERAGE "Enable coverage reporting" OFF)
    cmake_dependent_option(
        tyvi_ENABLE_GLOBAL_HARDENING
        "Attempt to push hardening options to built dependencies"
        ON
        tyvi_ENABLE_HARDENING
        OFF
    )

    tyvi_check_sanitizer_support()

    if(NOT PROJECT_IS_TOP_LEVEL OR tyvi_PACKAGING_MAINTAINER_MODE)
        option(tyvi_ENABLE_IPO "Enable IPO/LTO" OFF)
        option(tyvi_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
        option(tyvi_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
        option(tyvi_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
        option(tyvi_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
        option(tyvi_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
        option(tyvi_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
        option(tyvi_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
        option(tyvi_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
        option(tyvi_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
        option(tyvi_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
        option(tyvi_ENABLE_CACHE "Enable ccache" OFF)
    else()
        option(tyvi_ENABLE_IPO "Enable IPO/LTO" ON)
        option(tyvi_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
        option(tyvi_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
        option(tyvi_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
        option(tyvi_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
        option(tyvi_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
        option(tyvi_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
        option(tyvi_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
        option(tyvi_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
        option(tyvi_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
        option(tyvi_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
        option(tyvi_ENABLE_CACHE "Enable ccache" OFF)
    endif()

    if(NOT PROJECT_IS_TOP_LEVEL)
        mark_as_advanced(
            tyvi_ENABLE_IPO
            tyvi_WARNINGS_AS_ERRORS
            tyvi_ENABLE_USER_LINKER
            tyvi_ENABLE_SANITIZER_ADDRESS
            tyvi_ENABLE_SANITIZER_LEAK
            tyvi_ENABLE_SANITIZER_UNDEFINED
            tyvi_ENABLE_SANITIZER_THREAD
            tyvi_ENABLE_SANITIZER_MEMORY
            tyvi_ENABLE_UNITY_BUILD
            tyvi_ENABLE_CLANG_TIDY
            tyvi_ENABLE_CPPCHECK
            tyvi_ENABLE_COVERAGE
            tyvi_ENABLE_CACHE
        )
    endif()
endmacro()

# Setup options for the project and all the dependencies.
macro(TYVI_SETUP_GLOBAL_OPTIONS)
    if(tyvi_ENABLE_IPO)
        include(cmake/InterproceduralOptimization.cmake)
        tyvi_configure_ipo()
    endif()

    tyvi_check_sanitizer_support()

    if(tyvi_ENABLE_HARDENING AND tyvi_ENABLE_GLOBAL_HARDENING)
        include(cmake/Hardening.cmake)
        if(NOT SUPPORTS_UBSAN
           OR tyvi_ENABLE_SANITIZER_UNDEFINED
           OR tyvi_ENABLE_SANITIZER_ADDRESS
           OR tyvi_ENABLE_SANITIZER_THREAD
           OR tyvi_ENABLE_SANITIZER_LEAK
        )
            set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
        else()
            set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
        endif()
        message(STATUS "hardening: ${tyvi_ENABLE_HARDENING}")
        message(STATUS "ubsan minimal runtime: ${ENABLE_UBSAN_MINIMAL_RUNTIME}")
        message(STATUS "sanitizer undefined: ${tyvi_ENABLE_SANITIZER_UNDEFINED}")
        tyvi_setup_hardening(tyvi_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
    endif()
endmacro()

# Setup options for the project.
macro(TYVI_SETUP_LOCAL_OPTIONS)
    if(PROJECT_IS_TOP_LEVEL)
        include(cmake/StandardProjectSettings.cmake)
    endif()

    add_library(tyvi_warnings INTERFACE)
    add_library(tyvi_options INTERFACE)

    include(cmake/CompilerWarnings.cmake)
    tyvi_setup_target_warnings(tyvi_warnings ${tyvi_WARNINGS_AS_ERRORS})

    if(tyvi_ENABLE_USER_LINKER)
        include(cmake/Linker.cmake)
        tyvi_configure_target_linker(tyvi_options)
    endif()

    include(cmake/Sanitizers.cmake)
    tyvi_setup_target_sanitizers(
        tyvi_options
        ${tyvi_ENABLE_SANITIZER_ADDRESS}
        ${tyvi_ENABLE_SANITIZER_LEAK}
        ${tyvi_ENABLE_SANITIZER_UNDEFINED}
        ${tyvi_ENABLE_SANITIZER_THREAD}
        ${tyvi_ENABLE_SANITIZER_MEMORY}
    )

    set_target_properties(tyvi_options PROPERTIES UNITY_BUILD ${tyvi_ENABLE_UNITY_BUILD})

    if(tyvi_ENABLE_CACHE)
        include(cmake/Cache.cmake)
        tyvi_enable_cache()
    endif()

    include(cmake/StaticAnalyzers.cmake)
    if(tyvi_ENABLE_CLANG_TIDY)
        tyvi_setup_clang_tidy(${tyvi_WARNINGS_AS_ERRORS})
    endif()

    if(tyvi_ENABLE_CPPCHECK)
        tyvi_setup_cppcheck(${tyvi_WARNINGS_AS_ERRORS} "")
    endif()

    if(tyvi_ENABLE_COVERAGE)
        include(cmake/Tests.cmake)
        tyvi_setup_target_coverage(tyvi_options)
    endif()

    if(tyvi_WARNINGS_AS_ERRORS)
        check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
        if(LINKER_FATAL_WARNINGS)
            # This is not working consistently, so disabling for now
            # target_link_options(tyvi_options INTERFACE -Wl,--fatal-warnings)
        endif()
    endif()

    if(tyvi_ENABLE_HARDENING AND NOT tyvi_ENABLE_GLOBAL_HARDENING)
        include(cmake/Hardening.cmake)
        if(NOT SUPPORTS_UBSAN
           OR tyvi_ENABLE_SANITIZER_UNDEFINED
           OR tyvi_ENABLE_SANITIZER_ADDRESS
           OR tyvi_ENABLE_SANITIZER_THREAD
           OR tyvi_ENABLE_SANITIZER_LEAK
        )
            set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
        else()
            set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
        endif()
        tyvi_setup_hardening(tyvi_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
    endif()
endmacro()

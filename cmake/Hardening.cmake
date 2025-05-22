include(CheckCXXCompilerFlag)

# Setups hardening for target.
# If global == ON then
macro(
    MYPROJECT_SETUP_HARDENING
    target
    global
    ubsan_minimal_runtime
)
    # cmake-lint: disable=R0912,R0915
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang|GNU"))
        message(WARNING "Hardening not supported for: ${CMAKE_CXX_COMPILER_ID}")
        return()
    endif()

    if(${global})
        message(STATUS "** Enabling Hardening globally. **")
    else()
        message(STATUS "** Enabling Hardening (Target ${target}) **")
    endif()

    set(NEW_CXX_DEFINITIONS "${NEW_CXX_DEFINITIONS} -D_GLIBCXX_ASSERTIONS")
    message(STATUS "*** GLIBC++ Assertions (vector[], string[], ...) enabled")

    if(NOT
       CMAKE_BUILD_TYPE
       MATCHES
       "Debug"
    )
        set(NEW_COMPILE_OPTIONS "${NEW_COMPILE_OPTIONS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3")
    endif()
    message(STATUS "*** g++/clang _FORTIFY_SOURCE=3 enabled")

    #    check_cxx_compiler_flag(-fpie PIE)
    #if(PIE)
    #  set(NEW_COMPILE_OPTIONS ${NEW_COMPILE_OPTIONS} -fpie)
    #  set(NEW_LINK_OPTIONS ${NEW_LINK_OPTIONS} -pie)
    #
    #  message(STATUS "*** g++/clang PIE mode enabled")
    #else()
    #  message(STATUS "*** g++/clang PIE mode NOT enabled (not supported)")
    #endif()

    check_cxx_compiler_flag(-fstack-protector-strong STACK_PROTECTOR)
    if(STACK_PROTECTOR)
        set(NEW_COMPILE_OPTIONS "${NEW_COMPILE_OPTIONS} -fstack-protector-strong")
        message(STATUS "*** g++/clang -fstack-protector-strong enabled")
    else()
        message(STATUS "*** g++/clang -fstack-protector-strong NOT enabled (not supported)")
    endif()

    check_cxx_compiler_flag(-fcf-protection CF_PROTECTION)
    if(CF_PROTECTION)
        set(NEW_COMPILE_OPTIONS "${NEW_COMPILE_OPTIONS} -fcf-protection")
        message(STATUS "*** g++/clang -fcf-protection enabled")
    else()
        message(STATUS "*** g++/clang -fcf-protection NOT enabled (not supported)")
    endif()

    check_cxx_compiler_flag(-fstack-clash-protection CLASH_PROTECTION)
    if(CLASH_PROTECTION)
        if(LINUX OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            set(NEW_COMPILE_OPTIONS "${NEW_COMPILE_OPTIONS} -fstack-clash-protection")
            message(STATUS "*** g++/clang -fstack-clash-protection enabled")
        else()
            message(
                STATUS "*** g++/clang -fstack-clash-protection NOT enabled (clang on non-Linux)"
            )
        endif()
    else()
        message(STATUS "*** g++/clang -fstack-clash-protection NOT enabled (not supported)")
    endif()

    if(${ubsan_minimal_runtime})
        check_cxx_compiler_flag(
            "-fsanitize=undefined -fno-sanitize-recover=undefined -fsanitize-minimal-runtime"
            MINIMAL_RUNTIME
        )
        if(MINIMAL_RUNTIME)
            set(NEW_COMPILE_OPTIONS
                "${NEW_COMPILE_OPTIONS} -fsanitize=undefined -fsanitize-minimal-runtime"
            )
            set(NEW_LINK_OPTIONS
                "${NEW_LINK_OPTIONS} -fsanitize=undefined -fsanitize-minimal-runtime"
            )

            if(NOT ${global})
                set(NEW_COMPILE_OPTIONS "${NEW_COMPILE_OPTIONS} -fno-sanitize-recover=undefined")
                set(NEW_LINK_OPTIONS "${NEW_LINK_OPTIONS} -fno-sanitize-recover=undefined")
            else()
                message(
                    STATUS "** not enabling -fno-sanitize-recover=undefined for global consumption"
                )
            endif()

            message(STATUS "*** ubsan minimal runtime enabled")
        else()
            message(STATUS "*** ubsan minimal runtime NOT enabled (not supported)")
        endif()
    else()
        message(STATUS "*** ubsan minimal runtime NOT enabled (not requested)")
    endif()

    message(STATUS "** Hardening Compiler Flags: ${NEW_COMPILE_OPTIONS}")
    message(STATUS "** Hardening Linker Flags: ${NEW_LINK_OPTIONS}")
    message(STATUS "** Hardening Compiler Defines: ${NEW_CXX_DEFINITIONS}")

    if(${global})
        message(STATUS "** Setting hardening options globally for all dependencies")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NEW_COMPILE_OPTIONS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${NEW_LINK_OPTIONS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NEW_CXX_DEFINITIONS}")
    else()
        target_compile_options(${target} INTERFACE ${NEW_COMPILE_OPTIONS})
        target_link_options(${target} INTERFACE ${NEW_LINK_OPTIONS})
        target_compile_definitions(${target} INTERFACE ${NEW_CXX_DEFINITIONS})
    endif()
endmacro()

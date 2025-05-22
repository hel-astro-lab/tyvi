# Setup sanitizers from arguments for target.
function(
    myproject_setup_project_sanitizers
    target
    enable_sanitizer_address
    enable_sanitizer_leak
    enable_sanitizer_undefined_behavior
    enable_sanitizer_thread
    enable_sanitizer_memory
)

    set(enabled_sanitizers "")

    if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang"))
        message(STATUS "Sanitizers not enabled on unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
        return()
    endif()

    if(${enable_sanitizer_address})
        list(APPEND enabled_sanitizers "address")
    endif()

    if(${enable_sanitizer_leak})
        list(APPEND enabled_sanitizers "leak")
    endif()

    if(${enable_sanitizer_undefined_behavior})
        list(APPEND enabled_sanitizers "undefined")
    endif()

    if(${enable_sanitizer_thread})
        if("address" IN_LIST enabled_sanitizers OR "leak" IN_LIST enabled_sanitizers)
            message(
                WARNING "Thread sanitizer does not work with Address and Leak sanitizer enabled"
            )
        else()
            list(APPEND enabled_sanitizers "thread")
        endif()
    endif()

    if(${enable_sanitizer_memory} AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        message(
            WARNING
                [[
Memory sanitizer requires all the code (including libc++) to be msan-instrumented,
otherwise it reports false positives.
]]
        )
        if("address" IN_LIST enabled_sanitizers
           OR "thread" IN_LIST enabled_sanitizers
           OR "leak" IN_LIST enabled_sanitizers
        )
            message(
                WARNING
                    "Memory sanitizer does not work with Address, Thread or Leak sanitizer enabled"
            )
        else()
            list(APPEND enabled_sanitizers "memory")
        endif()
    endif()

    list(
        JOIN
        enabled_sanitizers
        ","
        list_of_enabled_sanitizers
    )

    if(list_of_enabled_sanitizers)
        if(NOT
           "${list_of_enabled_sanitizers}"
           STREQUAL
           ""
        )
            target_compile_options(${target} INTERFACE -fsanitize=${list_of_enabled_sanitizers})
            target_link_options(${target} INTERFACE -fsanitize=${list_of_enabled_sanitizers})
        endif()
    endif()
endfunction()

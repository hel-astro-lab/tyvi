# Setups cppcheck for the project.
macro(TYVI_SETUP_CPPCHECK warnings_as_errors cppcheck_options)
    find_program(CPPCHECK cppcheck)
    if(CPPCHECK)

        if(CMAKE_GENERATOR MATCHES ".*Visual Studio.*")
            set(CPPCHECK_TEMPLATE "vs")
        else()
            set(CPPCHECK_TEMPLATE "gcc")
        endif()

        if("${cppcheck_options}" STREQUAL "")
            # Enable all warnings that are actionable by the user of this toolset
            # style should enable the other 3, but we'll be explicit just in case
            set(SUPPRESS_DIR "*:${CMAKE_CURRENT_BINARY_DIR}/_deps/*")
            message(STATUS "cppcheck_options suppress: ${SUPPRESS_DIR}")
            set(CMAKE_CXX_CPPCHECK
                ${CPPCHECK}
                --template=${CPPCHECK_TEMPLATE}
                --enable=style,performance,warning,portability
                --inline-suppr
                # We cannot act on a bug/missing feature of cppcheck
                --suppress=cppcheckError
                --suppress=internalAstError
                # if a file does not have an internalAstError, we get an unmatchedSuppression error
                --suppress=unmatchedSuppression
                # noisy and incorrect sometimes
                --suppress=passedByValue
                # ignores code that cppcheck thinks is invalid C++
                --suppress=syntaxError
                --suppress=preprocessorErrorDirective
                # ignores static_assert type failures
                --suppress=knownConditionTrueFalse
                # explicit/deducing this parameter breaks this
                --suppress=functionStatic
                --inconclusive
                --suppress=${SUPPRESS_DIR}
            )
        else()
            # If the user provides a cppcheck_options with a template specified,
            # it will override this template.
            set(CMAKE_CXX_CPPCHECK ${CPPCHECK} --template=${CPPCHECK_TEMPLATE} ${cppcheck_options})
        endif()

        if(NOT
           "${CMAKE_CXX_STANDARD}"
           STREQUAL
           ""
        )
            set(CMAKE_CXX_CPPCHECK ${CMAKE_CXX_CPPCHECK} --std=c++${CMAKE_CXX_STANDARD})
        endif()
        if(${warnings_as_errors})
            list(APPEND CMAKE_CXX_CPPCHECK --error-exitcode=2)
        endif()
    else()
        message(${WARNING_MESSAGE} "cppcheck requested but executable not found")
    endif()
endmacro()

# Setups clang-tidy for the project.
macro(TYVI_SETUP_CLANG_TIDY warnings_as_errors)
    find_program(CLANGTIDY clang-tidy)
    if(CLANGTIDY)
        # construct the clang-tidy command line
        set(CLANG_TIDY_OPTIONS
            ${CLANGTIDY}
            -extra-arg=-Wno-unknown-warning-option
            -extra-arg=-Wno-ignored-optimization-argument
            -extra-arg=-Wno-unused-command-line-argument
            -p
        )
        # set standard
        if(NOT
           "${CMAKE_CXX_STANDARD}"
           STREQUAL
           ""
        )
            if("${CLANG_TIDY_OPTIONS_DRIVER_MODE}" STREQUAL "cl")
                set(CLANG_TIDY_OPTIONS ${CLANG_TIDY_OPTIONS}
                                       -extra-arg=/std:c++${CMAKE_CXX_STANDARD}
                )
            else()
                set(CLANG_TIDY_OPTIONS ${CLANG_TIDY_OPTIONS}
                                       -extra-arg=-std=c++${CMAKE_CXX_STANDARD}
                )
            endif()
        endif()

        # set warnings as errors
        if(${warnings_as_errors})
            list(APPEND CLANG_TIDY_OPTIONS -warnings-as-errors=*)
        endif()

        message(STATUS "clang-tidy enabled")
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_OPTIONS})
    else()
        message(${WARNING_MESSAGE} "clang-tidy requested but executable not found")
    endif()
endmacro()

# Setups include-what-you-use for the project.
macro(TYVI_SETUP_INCLUDE_WHAT_YOU_USE)
    find_program(INCLUDE_WHAT_YOU_USE include-what-you-use)
    if(INCLUDE_WHAT_YOU_USE)
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE})
    else()
        message(${WARNING_MESSAGE} "include-what-you-use requested but executable not found")
    endif()
endmacro()

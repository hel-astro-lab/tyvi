# from here:
#
# https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md

# Set warnings for target.
# Supports GCC and Clang based compilers.
#
# warnings_as_errors = <ON|OFF>
function(myproject_setup_project_warnings target warnings_as_errors)

    set(clang_warnings
        -Wall
        # reasonable and standard
        -Wextra
        # warn the user if a variable declaration shadows one from a parent context
        -Wshadow
        # warn the user if a class with virtual functions has a non-virtual destructor
        # This helps catch hard to track down memory errors.
        -Wnon-virtual-dtor
        # warn for c-style casts
        -Wold-style-cast
        # warn for potential performance problem casts
        -Wcast-align
        # warn on anything being unused
        -Wunused
        # warn if you overload (not override) a virtual function
        -Woverloaded-virtual
        # warn if non-standard C++ is used
        -Wpedantic
        # warn on type conversions that may lose data
        -Wconversion
        # warn on sign conversions
        -Wsign-conversion
        # warn if a null dereference is detected
        -Wnull-dereference
        # warn if float is implicit promoted to double
        -Wdouble-promotion
        # warn on security issues around functions that format output (ie printf)
        -Wformat=2
        # warn on statements that fallthrough without an explicit annotation
        -Wimplicit-fallthrough
    )

    set(gcc_warnings
        ${clang_warnings}
        # warn if indentation implies blocks where blocks do not exist
        -Wmisleading-indentation
        # warn if if / else chain has duplicated conditions
        -Wduplicated-cond
        # warn if if / else branches have duplicated code
        -Wduplicated-branches
        # warn about logical operations being used where bitwise were probably wanted
        -Wlogical-op
        # warn if you perform a cast to the same type
        -Wuseless-cast
        # warn if an overridden member function is not marked 'override' or 'final'
        -Wsuggest-override
    )

    if(warnings_as_errors)
        message(TRACE "Warnings are treated as errors")
        list(APPEND clang_warnings -Werror)
        list(APPEND gcc_warnings -Werror)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(target_project_warnings_cxx ${clang_warnings})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(target_project_warnings_cxx ${gcc_warnings})
    else()
        message(
            AUTHOR_WARNING "No compiler warnings set for CXX compiler: '${CMAKE_CXX_COMPILER_ID}'"
        )
        # TODO support Intel compiler
    endif()

    # use the same warning flags for C
    set(target_project_warnings_c "${target_project_warnings_cxx}")

    target_compile_options(
        ${target}
        INTERFACE # C++ warnings
                  $<$<COMPILE_LANGUAGE:CXX>:${target_project_warnings_cxx}>
                  # C warnings
                  $<$<COMPILE_LANGUAGE:C>:${target_project_warnings_c}>
    )
endfunction()

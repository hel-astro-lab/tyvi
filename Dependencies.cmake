include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other targets.
function(tyvi_setup_dependencies)

    # For each dependency, see if it's
    # already been provided to us by a parent project.

    if(NOT TARGET Boost::ut
       AND PROJECT_IS_TOP_LEVEL
       AND BUILD_TESTING
    )
        cpmaddpackage("gh:boost-ext/ut#v2.3.1")
    endif()

    if(NOT TARGET roc::rocprim AND ${tyvi_BACKEND} STREQUAL "hip")
        # rocThrust requires rocPRIM
        # "/opt/rocm" - default install prefix
        find_package(
            rocprim
            REQUIRED
            CONFIG
            PATHS
            "/opt/rocm/rocprim"
        )
    endif()

    if(NOT TARGET roc::rocthrust)
        if(${tyvi_BACKEND} STREQUAL "hip")
            # "/opt/rocm" - default install prefix
            find_package(
                rocthrust
                REQUIRED
                CONFIG
                PATHS
                "/opt/rocm/rocthrust"
            )
        elseif(${tyvi_BACKEND} STREQUAL "cpu")
            # "/opt/rocm" - default install prefix
            find_package(
                rocthrust
                REQUIRED
                CONFIG
                PATHS
                "/opt/rocm/rocthrust"
            )
        else()
            message(FATAL_ERROR "Unregonized tyvi_BACKEND: ${tyvi_BACKEND}")
        endif()
    endif()

    if(NOT TARGET std::mdspan)
        cpmaddpackage(
            NAME
            mdspan
            GIT_TAG
            stable
            GITHUB_REPOSITORY
            "kokkos/mdspan"
            OPTIONS
            "MDSPAN_GENERATE_STD_NAMESPACE_TARGETS ON"
        )
    endif()

endfunction()

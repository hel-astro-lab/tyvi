include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(tyvi_setup_dependencies)

    # For each dependency, see if it's
    # already been provided to us by a parent project

    if(NOT TARGET spdlog::spdlog)
        cpmaddpackage(
            NAME
            spdlog
            VERSION
            1.15.2
            GITHUB_REPOSITORY
            "gabime/spdlog"
            # bundeled fmt does not compile on hip
            # due to: https://github.com/llvm/llvm-project/issues/141592#issue-3093810283
            OPTIONS
            "SPDLOG_USE_STD_FORMAT ON"
        )
    endif()

    if(NOT TARGET Boost::ut)
        cpmaddpackage("gh:boost-ext/ut#v2.3.1")
    endif()

    if(NOT TARGET roc::rocprim)
        # On ROCm rocThrust requires rocPRIM
        find_package(
            rocprim
            REQUIRED
            CONFIG
            PATHS
            "/opt/rocm/rocprim"
        )
    endif()

    if(NOT TARGET roc::rocthrust)
        # "/opt/rocm" - default install prefix
        find_package(
            rocthrust
            REQUIRED
            CONFIG
            PATHS
            "/opt/rocm/rocthrust"
        )
    endif()

    if(NOT TARGET std::mdspan)
        # "/opt/rocm" - default install prefix
        cpmaddpackage(
            NAME
            mdspan
            GIT_TAG
            stable
            GITHUB_REPOSITORY
            "kokkos/mdspan"
        )
    endif()

endfunction()

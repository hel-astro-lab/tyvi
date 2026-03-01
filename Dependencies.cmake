include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other targets.
function(tyvi_setup_dependencies)

    # Hide clang-tidy and cppcheck from dependencies.
    # These are re-assigned at the end of this function.
    set(old_cmake_cxx_clang_tidy "${CMAKE_CXX_CLANG_TIDY}")
    set(old_cmake_cxx_cppcheck "${CMAKE_CXX_CPPCHECK}")
    unset(CMAKE_CXX_CLANG_TIDY)
    unset(CMAKE_CXX_CPPCHECK)

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

    if(${tyvi_BACKEND} STREQUAL "hip" AND NOT TARGET whip::whip)
        # pika explicitly wants whip 0.1.0
        cpmaddpackage(
            NAME
            whip
            GIT_TAG
            0.1.0
            GITHUB_REPOSITORY
            "eth-cscs/whip"
            OPTIONS
            "WHIP_BACKEND HIP"
        )
    endif()

    # todo: way to optimize boost and pika 'Performing Test ...'
    if(NOT TARGET pika::pika)

        # Compatibility alias for pika that expects Boost::boost.
        if(TARGET Boost::headers AND NOT TARGET Boost::boost)
            add_library(Boost::boost INTERFACE IMPORTED)
            target_link_libraries(Boost::boost INTERFACE Boost::headers)
        endif()

        # pika gets confused about whip when it is used through cpm.
        if(${tyvi_BACKEND} STREQUAL "hip" AND NOT TARGET whip::whip)
            add_library(whip::whip INTERFACE IMPORTED)
            target_link_libraries(whip::whip INTERFACE whip)
        endif()

        set(tyvi_enable_boost_context "OFF")
  if (APPLE)
    # Required by: https://pikacpp.org/usage.html
        set(tyvi_enable_boost_context "ON")

    endif()

        cpmaddpackage(
            NAME
            pika
            GIT_TAG
            0.34.0
            GITHUB_REPOSITORY
            "pika-org/pika"
            OPTIONS
            "PIKA_WITH_MALLOC system" # FIXME: don't use system malloc.
            "PIKA_WITH_HIP ON"
            "PIKA_WITH_MPI ON"
            "PIKA_WITH_BOOST_CONTEXT ${tyvi_enable_boost_context}"
            "PIKA_WITH_CXX_STANDARD 26"
        )
    endif()

    # cmake-lint: disable=C0103
    set(CMAKE_CXX_CLANG_TIDY "${old_cmake_cxx_clang_tidy}")
    set(CMAKE_CXX_CPPCHECK "${old_cmake_cxx_cppcheck}")
    # cmake-lint: disable=C0103
endfunction()

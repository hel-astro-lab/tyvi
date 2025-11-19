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

    if(NOT TARGET Boost::boost)
        set(boost_url_prefix "https://github.com/boostorg/boost/releases/download/")
        set(boost_release "boost-1.86.0/boost-1.86.0-cmake.tar.xz")
        cpmaddpackage(
            NAME
            Boost
            VERSION
            1.86.0 # Versions less than 1.85.0 may need patches for installation targets.
            URL
            "${boost_url_prefix}${boost_release}"
            URL_HASH
            SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
            OPTIONS
            "BOOST_ENABLE_CMAKE ON"
            "BOOST_SKIP_INSTALL_RULES ON" # Set `OFF` for installation
            "BUILD_SHARED_LIBS OFF"
            "BOOST_INCLUDE_LIBRARIES context" # Note the escapes!
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

        # Compatibility alias for pika that expects Boost::disable_autolink.
        if(NOT TARGET Boost::disable_autolinking)
            add_library(Boost::disable_autolinking INTERFACE IMPORTED)
            target_compile_definitions(
                Boost::disable_autolinking INTERFACE $<$<CXX_COMPILER_ID:MSVC>:BOOST_ALL_NO_LIB>
            )
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
endfunction()

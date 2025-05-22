include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(myproject_setup_dependencies)

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
            OPTIONS
            "SPDLOG_USE_STD_FORMAT ON"
        )
    endif()

    if(NOT TARGET Boost::ut)
        cpmaddpackage("gh:boost-ext/ut#v2.3.1")
    endif()

endfunction()

# Provide a test to verify that the version being reported from the application
# matches the version given to CMake. This will be important once you package
# your program. Real world shows that this is the kind of simple mistake that is easy
# to make, but also easy to test for.
# add_test(NAME cli.version_matches COMMAND intro --version)
# set_tests_properties(cli.version_matches PROPERTIES PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")

set(TESTS_TO_RUN
    unit_testing
    tyvi_compilation
    sstd
    mdgrid
    mdgrid_work
    mdgrid_buffer
    mdgrid_buffer_resize
)

add_library(tyvi_constant_testing)
target_include_directories(tyvi_constant_testing PUBLIC ".")
target_sources(
    tyvi_constant_testing
    PRIVATE constant_testing.c++
    PUBLIC FILE_SET
           constant_testing_headers
           TYPE
           HEADERS
           FILES
           constant_testing.h
)

target_link_libraries(tyvi_constant_testing PRIVATE tyvi_warnings tyvi_options Boost::ut)

foreach(test ${TESTS_TO_RUN})

    add_executable("${test}.test" "test_${test}.c++")

    target_link_libraries(
        "${test}.test"
        PRIVATE tyvi_warnings
                tyvi_options
                tyvi_constant_testing
                tyvi
                Boost::ut
    )

    if(${tyvi_BACKEND} STREQUAL "hip")
        tyvi_target_link_system_libraries("${test}.test" PRIVATE roc::rocthrust)
    endif()

    add_test(NAME ${test} COMMAND "${test}.test")
endforeach()

# Provide a simple smoke test to make sure that the CLI works and can display a --help message.
add_test(NAME unit_test_cli COMMAND unit_testing.test --help)
set_property(TEST unit_test_cli PROPERTY PASS_REGULAR_EXPRESSION "help")

find_package(MPI REQUIRED)

# Add a test which should be run on given amount of ranks.
function(add_multirank_test name ranks)
    add_executable("${name}.multirank_test" "multirank_test_${name}.c++")

    target_link_libraries(
        "${name}.multirank_test"
        PRIVATE tyvi_warnings
                tyvi_options
                tyvi
                Boost::ut
                roc::rocthrust
    )

    add_test(NAME "multirank-${name}" COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG}
                                              ${ranks} "${name}.multirank_test"
    )
endfunction(add_multirank_test)

add_multirank_test(pika 2)

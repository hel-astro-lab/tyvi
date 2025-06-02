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
    mdgrid_buffer
)

foreach(test ${TESTS_TO_RUN})

    add_executable("${test}.test" "test_${test}.c++")

    target_link_libraries(
        "${test}.test"
        PRIVATE tyvi_warnings
                tyvi_options
                tyvi
                Boost::ut
                roc::rocthrust
    )

    add_test(NAME ${test} COMMAND "${test}.test")
endforeach()

# Provide a simple smoke test to make sure that the CLI works and can display a --help message.
add_test(NAME unit_test_cli COMMAND unit_testing.test --help)
set_property(TEST unit_test_cli PROPERTY PASS_REGULAR_EXPRESSION "help")

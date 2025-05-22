# Provide a test to verify that the version being reported from the application
# matches the version given to CMake. This will be important once you package
# your program. Real world shows that this is the kind of simple mistake that is easy
# to make, but also easy to test for.
# add_test(NAME cli.version_matches COMMAND intro --version)
# set_tests_properties(cli.version_matches PROPERTIES PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")

add_executable(test_unit_testing test_unit_testing.c++)
target_link_libraries(
    test_unit_testing
    PRIVATE tyvi_warnings
            tyvi_options
            sample_library
            Boost::ut
)

# Provide a simple smoke test to make sure that the CLI works and can display a --help message
add_test(NAME unit_test_cli COMMAND test_unit_testing --help)
add_test(NAME unit_testing COMMAND test_unit_testing)

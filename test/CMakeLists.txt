# Tests that run on both CPU and HIP backends
set(COMMON_TESTS
    unit_testing
    sstd
    mdgrid_buffer_resize
    algo
)

# Tests that only run on the HIP backend
set(HIP_TESTS
    tyvi_compilation_hip
    mdgrid
    mdgrid_work
    mdgrid_buffer
)

# Tests that only run on the CPU backend
set(CPU_TESTS
    dynamic_array
    mdgrid_buffer_cpu
    mdgrid_cpu
    mdgrid_work_cpu
)

if(TYVI_BACKEND STREQUAL "cpu")
    set(TESTS_TO_RUN ${COMMON_TESTS} ${CPU_TESTS})
else()
    set(TESTS_TO_RUN ${COMMON_TESTS} ${HIP_TESTS})
endif()

foreach(test ${TESTS_TO_RUN})

    add_executable("${test}.test" "test_${test}.c++")

    target_link_libraries(
        "${test}.test"
        PRIVATE tyvi_warnings
                tyvi_options
                tyvi
                Boost::ut
    )

    if(TYVI_BACKEND STREQUAL "hip")
        target_link_libraries("${test}.test" PRIVATE roc::rocthrust)
    endif()

    add_test(NAME ${test} COMMAND "${test}.test")
endforeach()

# Provide a simple smoke test to make sure that the CLI works and can display a --help message.
add_test(NAME unit_test_cli COMMAND unit_testing.test --help)
set_property(TEST unit_test_cli PROPERTY PASS_REGULAR_EXPRESSION "help")

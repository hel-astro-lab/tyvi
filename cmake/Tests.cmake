# Setups coverage reporting for target.
function(tyvi_setup_target_coverage target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        target_compile_options(${target} INTERFACE --coverage -O0 -g)
        target_link_libraries(${target} INTERFACE --coverage)
    else()
        message(WARNING "coverage wanted but it's not supported by: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

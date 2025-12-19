add_library(tyvi)
add_library(tyvi::tyvi ALIAS tyvi)

target_include_directories(tyvi PUBLIC ".")

target_sources(
    tyvi
    PRIVATE tyvi/mdgrid_work.cpp
    PUBLIC FILE_SET
           all_headers
           TYPE
           HEADERS
           FILES
           tyvi/thrust_test.h
           tyvi/mdspan.h
           tyvi/sstd.h
           tyvi/mdgrid.h
           tyvi/mdgrid_buffer.h
           tyvi/backend.h
           tyvi/execution.h
)

target_link_libraries(tyvi PRIVATE tyvi_options tyvi_warnings)

target_compile_features(tyvi PUBLIC cxx_std_26)

set_target_properties(tyvi PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(tyvi PROPERTIES LINKER_LANGUAGE CXX)

# Use dependencies as system library to suppress warnings from it's header.
include(../cmake/SystemLink.cmake)
tyvi_target_link_system_libraries(
    tyvi
    PUBLIC
    std::mdspan
    roc::rocthrust
)

if(${tyvi_BACKEND} STREQUAL "hip")
    target_compile_definitions(tyvi PUBLIC TYVI_BACKEND_HIP)
elseif(${tyvi_BACKEND} STREQUAL "cpu")
    target_compile_definitions(tyvi PUBLIC TYVI_BACKEND_CPU)
    target_compile_options(tyvi PUBLIC -fopenmp-simd)
else()
    message(FATAL_ERROR "Unregonized tyvi_BACKEND: ${tyvi_BACKEND}")
endif()

# Compared to std::mdspan and roc::rocthrust,
# pika does not set INTERFACE_INCLUDE_DIRECTORIES,
# so the above is not enough to make -isystem appear.
add_library(pika_system INTERFACE)
target_link_libraries(pika_system INTERFACE pika::pika)
target_include_directories(
    pika_system SYSTEM INTERFACE $<TARGET_PROPERTY:pika::pika,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(tyvi PUBLIC pika_system)

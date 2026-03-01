add_library(tyvi)
add_library(tyvi::tyvi ALIAS tyvi)

target_include_directories(tyvi PUBLIC ".")

# Backend-conditional sources and headers
if(TYVI_BACKEND STREQUAL "cpu")
    target_sources(
        tyvi
        PRIVATE tyvi/mdgrid_work_cpu.cpp
        PUBLIC FILE_SET
               all_headers
               TYPE
               HEADERS
               FILES
               tyvi/backend.h
               tyvi/dynamic_array_cpu.h
               tyvi/algo.h
               tyvi/containers.h
               tyvi/iterators.h
               tyvi/mdspan.h
               tyvi/sstd.h
               tyvi/mdgrid.h
               tyvi/mdgrid_buffer.h
    )

    target_compile_definitions(tyvi PUBLIC TYVI_USE_CPU_BACKEND)
    target_compile_options(tyvi PUBLIC -fopenmp-simd)
else()
    target_compile_definitions(tyvi PUBLIC TYVI_USE_HIP_BACKEND)
    target_sources(
        tyvi
        PRIVATE tyvi/mdgrid_work_hip.cpp
        PUBLIC FILE_SET
               all_headers
               TYPE
               HEADERS
               FILES
               tyvi/backend.h
               tyvi/algo.h
               tyvi/containers.h
               tyvi/iterators.h
               tyvi/thrust_test_hip.h
               tyvi/mdspan.h
               tyvi/sstd.h
               tyvi/mdgrid.h
               tyvi/mdgrid_buffer.h
    )
endif()

target_link_libraries(tyvi PRIVATE tyvi_options tyvi_warnings)

target_compile_features(tyvi PUBLIC cxx_std_23)

set_target_properties(tyvi PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(tyvi PROPERTIES LINKER_LANGUAGE CXX)

# Use dependencies as system library to suppress warnings from it's header.
include(../cmake/SystemLink.cmake)

if(TYVI_BACKEND STREQUAL "cpu")
    tyvi_target_link_system_libraries(
        tyvi
        PUBLIC
        std::mdspan
    )
else()
    tyvi_target_link_system_libraries(
        tyvi
        PUBLIC
        std::mdspan
        roc::rocthrust
    )
endif()

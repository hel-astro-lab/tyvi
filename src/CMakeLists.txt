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
)

target_link_libraries(tyvi PRIVATE tyvi_options tyvi_warnings)

target_compile_features(tyvi PUBLIC cxx_std_23)

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

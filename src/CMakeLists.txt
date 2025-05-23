add_library(sample_library)
add_library(tyvi::sample_library ALIAS sample_library)

target_include_directories(sample_library PUBLIC ".")

target_sources(
    sample_library
    PRIVATE tyvi/sample_library.c++
    PUBLIC FILE_SET
           all_headers
           TYPE
           HEADERS
           FILES
           tyvi/sample_library.h
)

target_link_libraries(sample_library PRIVATE tyvi_options tyvi_warnings)

target_compile_features(sample_library PUBLIC cxx_std_23)

set_target_properties(sample_library PROPERTIES VERSION ${PROJECT_VERSION})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(sample_library PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()

# Use spdlog as system library to suppress warnings from it's header.
include(../cmake/SystemLink.cmake)
tyvi_target_link_system_libraries(sample_library PRIVATE spdlog::spdlog)

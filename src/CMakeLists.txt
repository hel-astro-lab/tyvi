add_library(sample_library)
add_library(myproject::sample_library ALIAS sample_library)

target_source(
    sample_library
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src/sample_library.c++
    PUBLIC
    FILE_SET
    all_headers
    TYPE
    HEADERS
    FILES
    ${PROJECT_SOURCE_DIR}/src/sample_library.h
)

target_link_libraries(sample_library PRIVATE myproject_options myproject_warnings)

target_compile_features(sample_library PUBLIC cxx_std_23)

set_target_properties(sample_library PROPERTIES VERSION ${PROJECT_VERSION})

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(sample_library PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()

# Test plugins CMakeLists.txt

# Simple Gain Plugin
add_library(simple_gain SHARED
    simple_gain/simple_gain.cpp
)

target_include_directories(simple_gain PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(simple_gain PRIVATE cxx_std_17)

set_target_properties(simple_gain PROPERTIES
    PREFIX ""
    SUFFIX ".dylib"
    OUTPUT_NAME "simple_gain"
)
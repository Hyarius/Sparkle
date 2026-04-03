add_executable(sparkleResourcesConverter main.cpp)

target_compile_features(sparkleResourcesConverter PRIVATE cxx_std_20)

set_target_properties(sparkleResourcesConverter PROPERTIES
    FOLDER tools
)

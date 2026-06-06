add_executable(sparkleResourcesConverter main.cpp)

target_compile_features(sparkleResourcesConverter PRIVATE cxx_std_23)

set_target_properties(sparkleResourcesConverter PROPERTIES
    FOLDER tools
)

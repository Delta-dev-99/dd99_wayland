
set(target_name dd99_wayland)
set(component_name wayland)
add_library(${target_name})
add_library(dd99::${component_name} ALIAS ${target_name})
set_target_warnings(${target_name} PRIVATE)
target_compile_features(${target_name} PUBLIC cxx_std_20)
target_include_directories(${target_name} PUBLIC include)
target_sources(${target_name} PRIVATE
    src/engine.cpp
)

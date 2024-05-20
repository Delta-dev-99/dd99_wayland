
find_package(pugixml REQUIRED)

set(target_name dd99_wayland_scanner)
add_executable(${target_name})
set_target_warnings(${target_name} PRIVATE)
target_compile_features(${target_name} PUBLIC cxx_std_23)
target_include_directories(${target_name} PUBLIC include)
target_sources(${target_name} PRIVATE
    scanner.cpp
)
target_link_libraries(${target_name} PRIVATE pugixml::pugixml)


set(current_target test1)
add_executable(${current_target} test1.cpp)
target_link_libraries(${current_target} PRIVATE dd99::wayland)
set_target_warnings(${current_target})

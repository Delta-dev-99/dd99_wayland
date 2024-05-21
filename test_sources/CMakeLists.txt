
set(current_target dd99_wayland_test1)
add_executable(${current_target} test1.cpp)
target_include_directories(${current_target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${current_target} PRIVATE dd99::wayland)
set_target_warnings(${current_target} PRIVATE)
dd99_add_wayland_client_protocol(${current_target} PROTOCOL /usr/share/wayland/wayland.xml BASENAME wayland)

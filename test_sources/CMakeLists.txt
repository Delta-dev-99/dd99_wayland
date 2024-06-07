
find_package(LibUring REQUIRED)

include(FetchContent)

FetchContent_Declare(asio
    GIT_REPOSITORY /home/dd99/Documents/gits/asio
    GIT_TAG master
    # GIT_TAG asio-1-28-2
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
set(FETCHCONTENT_UPDATES_DISCONNECTED_asio ON)


FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    message(STATUS "[asio] populating...")
    FetchContent_Populate(asio)
    message(STATUS "[asio] done")
endif()
add_library(asio INTERFACE)
add_library(asio::asio ALIAS asio)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio INTERFACE ASIO_HAS_IO_URING ASIO_DISABLE_EPOLL)
target_link_libraries(asio INTERFACE LibUring::LibUring)
# target_link_libraries(asio INTERFACE Threads::Threads)



set(current_target dd99_wayland_test1)
add_executable(${current_target} test1.cpp)
target_include_directories(${current_target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${current_target} PRIVATE dd99::wayland asio::asio)
# set_target_warnings(${current_target} PRIVATE)
dd99_add_wayland_client_protocol(${current_target} PROTOCOL /usr/share/wayland/wayland.xml BASENAME wayland)
dd99_add_wayland_client_protocol(${current_target} PROTOCOL /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml BASENAME xdg-shell)

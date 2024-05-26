cmake_minimum_required(VERSION 3.28.0)
project(dd99_wayland VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")
# include(build_utils/make_version.cmake)
include(cmake/CompilerWarnings.cmake)

add_subdirectory(dd99_wayland)
add_subdirectory(scanner)


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(DD99_WAYLAND_IS_MAIN_PROJECT TRUE)
endif(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

# check if we should enable tests
option(DD99_WAYLAND_ENABLE_TESTS "enable test targets of dd99_wayland project" ${DD99_WAYLAND_IS_MAIN_PROJECT})
if (DD99_WAYLAND_ENABLE_TESTS)
    # include(CTest)
    # enable_testing()
    add_subdirectory(test_sources EXCLUDE_FROM_ALL)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

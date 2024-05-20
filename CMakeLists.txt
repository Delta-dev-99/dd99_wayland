cmake_minimum_required(VERSION 3.28.0)
project(dd99_wayland VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")
# include(build_utils/make_version.cmake)
include(cmake/CompilerWarnings.cmake)

add_subdirectory(dd99_wayland)
add_subdirectory(scanner)
add_subdirectory(test_sources EXCLUDE_FROM_ALL)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

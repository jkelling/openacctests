cmake_minimum_required(VERSION 3.2)
project(openacctests)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(collectiveShared)
add_subdirectory(hostClass)

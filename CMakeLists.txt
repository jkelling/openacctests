cmake_minimum_required(VERSION 3.2)
project(openacctests)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

include_directories(include/)

add_subdirectory(collectiveShared)
add_subdirectory(hostClass)
add_subdirectory(apply)
add_subdirectory(lambda)
add_subdirectory(placementNew)
add_subdirectory(accMalloc)
add_subdirectory(workerLoop)
add_subdirectory(loop)
add_subdirectory(sharedMem)

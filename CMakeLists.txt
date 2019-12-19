cmake_minimum_required(VERSION 3.1.2)

project(tagreader VERSION 1.3.6)

list( APPEND CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS} -g -ftest-coverage -fprofile-arcs")

add_definitions(
        -Wfatal-errors
)

set(libtagreader_SOURCES
  src/tagreader.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/ericniebler/range-v3/include)

add_executable(tagreader ${libtagreader_SOURCES})

target_link_libraries(${PROJECT_NAME} stdc++fs)

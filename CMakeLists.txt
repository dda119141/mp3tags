cmake_minimum_required(VERSION 3.1.2)

project(tagreader VERSION 1.3.6)

list( APPEND CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS} -g -ftest-coverage -pthread -fprofile-arcs")

add_definitions(
        -Wall
        -Wfatal-errors
        -Werror
)

set(TEST_SOURCES
  tagreader.cpp
  tagwriter.cpp
)

set(EXAMPLES_SOURCES
  change_dir.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/catch2)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/ericniebler/range-v3/include)

foreach(APP ${TEST_SOURCES})
    string( REPLACE ".cpp" "" APP_BIN "${APP}" )
    add_executable(${APP_BIN} src/${APP})
    target_link_libraries(${APP_BIN} stdc++fs)
endforeach()

foreach(APP ${EXAMPLES_SOURCES})
    string( REPLACE ".cpp" "" APP_EXPLE "${APP}" )
    add_executable(${APP_EXPLE} examples/${APP})
    target_link_libraries(${APP_EXPLE} stdc++fs)
endforeach()


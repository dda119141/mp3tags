cmake_minimum_required(VERSION 3.1.2)

project(tagreader VERSION 1.3.6)

# Provide path for scripts
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(findComp)

list( APPEND CMAKE_CXX_FLAGS "${_CXX_VERSION} ${CMAKE_CXX_FLAGS} -g -ftest-coverage -pthread -fprofile-arcs")

if(${CYGWIN})
    # append this in order to avoid "/x86_64-pc-cygwin/bin/as ...too many sections" compile error
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

if (WIN32)
add_definitions(
        -Wall
        -g
)
else()
add_definitions(
        -Wall
		-Wfatal-errors
		-Werror
        -g
)
endif()

set(TEST_SOURCES
  tagwriter.cpp
)

set(EXAMPLES_SOURCES
  readTag.cpp
  writeTag.cpp
  handleDirectory.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/spdlog/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/lyra/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/catch/single_include/catch2)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/ericniebler/range-v3/include)


if (WIN32)
	foreach(APP ${TEST_SOURCES})
		string( REPLACE ".cpp" "" APP_BIN "${APP}" )
		add_executable(${APP_BIN} tests/${APP})
	endforeach()

	foreach(APP ${EXAMPLES_SOURCES})
		string( REPLACE ".cpp" "" APP_EXPLE "${APP}" )
		add_executable(${APP_EXPLE} examples/${APP})
	endforeach()
else()
	foreach(APP ${TEST_SOURCES})
		string( REPLACE ".cpp" "" APP_BIN "${APP}" )
		add_executable(${APP_BIN} tests/${APP})
		target_link_libraries(${APP_BIN} stdc++fs)
	endforeach()

    string( REPLACE ".cpp" "" APP_EXPLE "${APP}" )
    add_executable(${APP_EXPLE} examples/${APP})
    target_link_libraries(${APP_EXPLE} stdc++fs)
endif()


cmake_minimum_required(VERSION 3.9.0)

project(tagreader VERSION 1.3.6)

# Provide path for scripts
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(findComp)

if(${CYGWIN})
    # append this in order to avoid "/x86_64-pc-cygwin/bin/as ...too many sections" compile error
    target_compile_options(${PROJECT_NAME} PRIVATE "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()


set(TEST_SOURCES
  tagwriter.cpp
)

set(EXAMPLES_SOURCES
  readTag.cpp
  writeTag.cpp
  handleDirectory.cpp
)

function(ConfigureTarget _target)
	target_include_directories(${_target} PRIVATE 
			${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/spdlog/include
			${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/lyra/include
			${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/catch/single_include/catch2
			${CMAKE_CURRENT_SOURCE_DIR}/include/3rd-party/ericniebler/range-v3/include
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/include
	   )

	target_compile_options(${_target} PRIVATE
		 $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
         -Wall -Werror -g -ftest-coverage -pthread -fprofile-arcs -std=c++17>
	 #-Wall -Werror -Wfatal-errors -g -ftest-coverage -pthread -fprofile-arcs -std=c++17>
		 $<$<CXX_COMPILER_ID:MSVC>:
			  /W4 /std:c++latest>)

	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	  # using Clang
	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	  # using GCC
		target_link_libraries(${_target} PRIVATE stdc++fs gcov)
	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	  # using Visual Studio C++
	endif()

	target_compile_features(${_target} INTERFACE cxx_std_17)
endfunction()


foreach(APP ${TEST_SOURCES})
	string( REPLACE ".cpp" "" APP_BIN "${APP}" )
	add_executable(${APP_BIN} tests/${APP})
	ConfigureTarget(${APP_BIN})
endforeach()

foreach(APP ${EXAMPLES_SOURCES})
	string( REPLACE ".cpp" "" APP_EXPLE "${APP}" )
	add_executable(${APP_EXPLE} examples/${APP})
	ConfigureTarget(${APP_EXPLE})
endforeach()






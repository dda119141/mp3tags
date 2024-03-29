
# Don't add this line if you will try_compile with boost.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# test that filesystem header actually is there and works
#try_compile(HAS_FS "${CMAKE_BINARY_DIR}/temp"
#"${CMAKE_SOURCE_DIR}/cmake/tests/has_filesystem.cc"
#CMAKE_FLAGS ${_CXX_VERSION} LINK_LIBRARIES stdc++fs)

try_compile(HAS_FS_EXPERIMENTAL "${CMAKE_BINARY_DIR}/temp"
"${CMAKE_SOURCE_DIR}/cmake/tests/has_filesystem_experimental.cc"
CMAKE_FLAGS ${_CXX_VERSION}  LINK_LIBRARIES stdc++fs)

try_compile(HAS_FS_MSVC "${CMAKE_BINARY_DIR}/temp"
"${CMAKE_SOURCE_DIR}/cmake/tests/has_filesystem.cc"
CMAKE_FLAGS ${_CXX_VERSION})

if (HAS_FS)
    add_compile_options(-DHAS_FS)
    message(STATUS "Compiler has filesystem support")
elseif (HAS_FS_EXPERIMENTAL)
    add_compile_options(-DHAS_FS_EXPERIMENTAL)
    message(STATUS "Compiler has filesystem experimental support")
elseif (MSVC_VERSION GREATER_EQUAL "1900")
	message(STATUS "Compiler has filesystem experimental support")
else()
#   .... You could also try searching for boost::filesystem here.
    message(FATAL_ERROR "Compiler is missing filesystem capabilities")
endif()

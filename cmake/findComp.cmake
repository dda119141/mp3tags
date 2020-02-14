# set everything up for c++ 17 features
set(CMAKE_CXX_STANDARD 17)

# Don't add this line if you will try_compile with boost.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# test that filesystem header actually is there and works
try_compile(HAS_FS "${CMAKE_BINARY_DIR}/temp"
"${CMAKE_SOURCE_DIR}/cmake/tests/has_filesystem.cc"
            CMAKE_FLAGS -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON
            LINK_LIBRARIES stdc++fs)

        try_compile(HAS_FS_EXPERIMENTAL "${CMAKE_BINARY_DIR}/temp"
"${CMAKE_SOURCE_DIR}/cmake/tests/has_filesystem_experimental.cc"
            CMAKE_FLAGS -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON
            LINK_LIBRARIES stdc++fs)

if (HAS_FS)
    add_compile_options(-DHAS_FS)
    message(STATUS "Compiler has filesystem support")
elseif (HAS_FS_EXPERIMENTAL)
    add_compile_options(-DHAS_FS_EXPERIMENTAL)
    message(STATUS "Compiler has filesystem experimental support")
else()
#   .... You could also try searching for boost::filesystem here.
    message(FATAL_ERROR "Compiler is missing filesystem capabilities")
endif()

cmake_minimum_required(VERSION 3.10)
project(duplex_pipe C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)         

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()


set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra")

find_package(Threads REQUIRED)

add_library(duplex STATIC duplex.c)
target_include_directories(duplex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(echo_test main.c)
target_link_libraries(echo_test PRIVATE duplex Threads::Threads)

add_custom_target(testfile
    COMMAND dd if=/dev/urandom
               of=${CMAKE_CURRENT_BINARY_DIR}/test_4gb.bin
               bs=1048576 count=4096 status=progress
    COMMENT "Generate 4 GiB file txt..."
    VERBATIM
)
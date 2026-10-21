cmake_minimum_required(VERSION 3.10)
project(monte_carlo_threads C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra")

find_package(Threads REQUIRED)

add_library(calculator STATIC calculator.c)
target_include_directories(calculator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(threads main.c)
target_link_libraries(threads PRIVATE calculator Threads::Threads rt)

add_custom_target(plot
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.py
            --bin $<TARGET_FILE:threads>
            --out ${CMAKE_CURRENT_SOURCE_DIR}/tester/threads_plot.png
    DEPENDS threads
    COMMENT "Running Monte-Carlo thread scaling benchmark and building plot..."
    VERBATIM
)

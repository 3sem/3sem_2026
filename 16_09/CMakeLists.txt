cmake_minimum_required(VERSION 3.16)

project(ipc_transfer LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Common compiler warnings and POSIX interfaces.
add_library(ipc_transfer INTERFACE)

target_compile_options(ipc_transfer INTERFACE
    $<$<OR:$<C_COMPILER_ID:GNU>,
            $<C_COMPILER_ID:Clang>,
            $<C_COMPILER_ID:AppleClang>>:-Wall -Wextra -Wpedantic>
)

target_compile_definitions(ipc_transfer INTERFACE
    _POSIX_C_SOURCE=200809L
)

# IPC implementations.
add_subdirectory(SYS_V)
add_subdirectory(SYS_V_queue)
add_subdirectory(FIFO)

# Benchmarks.
option(BENCHMARKS "Build benchmarks" OFF)

if(BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

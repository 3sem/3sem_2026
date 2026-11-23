cmake_minimum_required(VERSION 3.10)

project(terminal_emulator C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_EXTENSIONS OFF)

add_compile_definitions(_POSIX_C_SOURCE=200809L)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")

    add_compile_options(-Wall -Wextra -Wpedantic)

endif()

add_executable(mysh

    main.c

    parser.c

    executor.c
)

enable_testing()

add_test(

    NAME single_command_test

    COMMAND sh -c "printf 'echo hello_cmake\nexit\n' | $<TARGET_FILE:mysh> | grep -q '^hello_cmake$'"

)

add_test(

    NAME pipeline_three_stages_test

    COMMAND sh -c "printf 'env | grep PATH | wc -l\nexit\n' | $<TARGET_FILE:mysh> | grep -q '^[[:space:]]*1$'"

)

add_test(

    NAME fd_minimization_long_pipeline_test

    COMMAND sh -c "printf 'seq 1 100 | grep 5 | sort -r | head -n 5 | wc -l\nexit\n' | $<TARGET_FILE:mysh> | grep -q '^[[:space:]]*5$'"

)


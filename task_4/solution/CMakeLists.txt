cmake_minimum_required(VERSION 3.16)

project(integralCalculator LANGUAGES C)

find_package(Threads REQUIRED)

add_library(expressionParser STATIC
    parser.c
    expression.c
    tree.c
)

add_library(common STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/fileBuffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/ipc/shMem.c)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../common)

target_include_directories(expressionParser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(expressionParser PUBLIC c_std_11)

add_executable(integralCalculator
    main.c
    modes.c
    calcIntegral.c
    resultTransfer.c
)

target_link_libraries(integralCalculator PRIVATE expressionParser common Threads::Threads)
target_compile_definitions(integralCalculator PRIVATE _POSIX_C_SOURCE=200809L)

if(MSVC)
    target_compile_options(expressionParser PRIVATE /W4)
    target_compile_options(integralCalculator PRIVATE /W4)
else()
    target_compile_options(expressionParser PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(integralCalculator PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(UNIX)
    target_link_libraries(expressionParser PUBLIC m)
endif()

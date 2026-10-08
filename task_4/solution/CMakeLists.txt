cmake_minimum_required(VERSION 3.16)

project(integralCalculator LANGUAGES C)

find_package(Threads REQUIRED)

add_library(expressionParser STATIC
    parser.c
    expression.c
    tree.c
)

target_include_directories(expressionParser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(expressionParser PUBLIC c_std_11)

add_executable(integralCalculator
    main.c
    calcIntegral.c
)

target_link_libraries(integralCalculator PRIVATE expressionParser Threads::Threads)

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

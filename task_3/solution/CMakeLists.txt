cmake_minimum_required(VERSION 3.24)

project(fileTransfer
    VERSION 1.0.0
    LANGUAGES C
)

add_library(linuxCmd STATIC
    ../../common/linuxCmd.c
)

target_include_directories(linuxCmd PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../common"
)

add_executable(ipcTransfer main.c)

target_link_libraries(ipcTransfer PRIVATE linuxCmd)

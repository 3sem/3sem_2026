cmake_minimum_required(VERSION 3.24)

project(fileTransfer
    VERSION 1.0.0
    LANGUAGES C
)

option(
    FIFO
    "Enable fifo transfer"
    OFF
)

add_library(common STATIC
    ../../common/linuxCmd.c
    ../../common/fileBuffer.c
    ../../common/ipc/fifo.c
)

target_include_directories(common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../common"
)

add_executable(ipcTransfer main.c)

target_link_libraries(ipcTransfer PRIVATE common)

if(FIFO)
    add_library(fifoModule STATIC
        fileTransfer/fifoTransfer.c
    )
    target_link_libraries(fifoModule PUBLIC common)
    target_link_libraries(ipcTransfer PRIVATE fifoModule)
endif()

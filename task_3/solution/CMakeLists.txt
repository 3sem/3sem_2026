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

option(
    SHMEM
    "Enable shared memory transfer"
    OFF
)

set(transfer_option_count 0)
foreach(transfer_option IN ITEMS FIFO SHMEM)
    if(${transfer_option})
        math(EXPR transfer_option_count "${transfer_option_count} + 1")
    endif()
endforeach()

if(NOT transfer_option_count EQUAL 1)
    message(FATAL_ERROR "Enable exactly one transfer option: FIFO or SHMEM")
endif()

add_library(common STATIC
    ../../common/linuxCmd.c
    ../../common/fileBuffer.c
)

target_include_directories(common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../common"
)

add_executable(ipcTransfer main.c)

target_link_libraries(ipcTransfer PRIVATE common)

if(FIFO)
    add_library(fifoModule STATIC
        fileTransfer/fifoTransfer.c
        ../../common/ipc/fifo.c
    )
    target_link_libraries(fifoModule PUBLIC common)
    target_link_libraries(ipcTransfer PRIVATE fifoModule)
elseif(SHMEM)
    add_library(shmemModule STATIC
        fileTransfer/shMemTransfer.c
        ../../common/ipc/shMem.c
    )
    target_link_libraries(shmemModule PUBLIC common)
    target_link_libraries(ipcTransfer PRIVATE shmemModule)
endif()

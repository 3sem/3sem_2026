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

set(transfer_config "${CMAKE_CURRENT_SOURCE_DIR}/transfer.conf")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${transfer_config}")
file(STRINGS "${transfer_config}" transfer_config_lines)
foreach(path_name IN ITEMS FIFO_PATH SHMEM_PATH)
    unset(${path_name})
    foreach(line IN LISTS transfer_config_lines)
        if(line MATCHES "^${path_name}=\"(/[A-Za-z0-9_./ -]+)\"$")
            set(${path_name} "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(NOT DEFINED ${path_name})
        message(FATAL_ERROR "Set ${path_name} to a double-quoted absolute path in transfer.conf (letters, digits, spaces, _, ., /, -)")
    endif()
endforeach()

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
    target_compile_definitions(fifoModule PRIVATE FIFO_PATH="${FIFO_PATH}")
    target_link_libraries(ipcTransfer PRIVATE fifoModule)
elseif(SHMEM)
    add_library(shmemModule STATIC
        fileTransfer/shMemTransfer.c
        ../../common/ipc/shMem.c
    )
    target_link_libraries(shmemModule PUBLIC common)
    target_compile_definitions(shmemModule PRIVATE SHMEM_PATH="${SHMEM_PATH}")
    target_link_libraries(ipcTransfer PRIVATE shmemModule)
endif()

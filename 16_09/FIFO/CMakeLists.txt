add_library(fifo_utils STATIC
    src/fifo_utils.c
)

target_include_directories(fifo_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(fifo_utils PUBLIC
    _POSIX_C_SOURCE=200809L
)

target_link_libraries(fifo_utils PUBLIC
    ipc_transfer
)

add_executable(fifo_sender
    src/sender.c
)

target_link_libraries(fifo_sender PRIVATE
    fifo_utils
)

add_executable(fifo_receiver
    src/receiver.c
)

target_link_libraries(fifo_receiver PRIVATE
    fifo_utils
)

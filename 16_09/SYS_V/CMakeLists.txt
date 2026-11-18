# Common System V IPC utilities
add_library(ipc_utils STATIC
    src/ipc_utils.c
)

target_include_directories(ipc_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ipc_utils PUBLIC
    ipc_transfer
)

# Sender
add_executable(sender
    src/sender.c
)

target_link_libraries(sender PRIVATE
    ipc_utils
)

# Receiver
add_executable(receiver
    src/receiver.c
)

target_link_libraries(receiver PRIVATE
    ipc_utils
)

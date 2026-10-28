add_executable(queue_sender
    src/sender.c
)

target_include_directories(queue_sender PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(queue_sender PRIVATE
    ipc_utils
)

add_executable(queue_receiver
    src/receiver.c
)

target_include_directories(queue_receiver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(queue_receiver PRIVATE
    ipc_utils
)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_program(MD5SUM_EXECUTABLE md5sum)

if(NOT MD5SUM_EXECUTABLE)
    message(FATAL_ERROR "md5sum was not found")
endif()

add_custom_target(run_benchmarks
    COMMAND bash
        "${CMAKE_CURRENT_SOURCE_DIR}/run_benchmark.sh"
        "$<TARGET_FILE:sender>"
        "$<TARGET_FILE:receiver>"
        "$<TARGET_FILE:queue_sender>"
        "$<TARGET_FILE:queue_receiver>"
        "$<TARGET_FILE:fifo_sender>"
        "$<TARGET_FILE:fifo_receiver>"
        "${CMAKE_BINARY_DIR}/benchmark-results"
        "${Python3_EXECUTABLE}"
        "${MD5SUM_EXECUTABLE}"
    DEPENDS
        sender
        receiver
        queue_sender
        queue_receiver
        fifo_sender
        fifo_receiver
    USES_TERMINAL
    COMMENT "Running shared-memory, message-queue and FIFO benchmarks"
    VERBATIM
)

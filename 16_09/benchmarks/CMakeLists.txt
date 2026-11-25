find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_program(MD5SUM_EXECUTABLE md5sum)

if(NOT MD5SUM_EXECUTABLE)
    message(FATAL_ERROR "md5sum was not found")
endif()

get_filename_component(
    RESULTS_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../results"
    ABSOLUTE
)

add_custom_target(run_benchmarks
    COMMAND bash
        "${CMAKE_CURRENT_SOURCE_DIR}/run_all_benchmarks.sh"
        "$<TARGET_FILE:sender>"
        "$<TARGET_FILE:receiver>"
        "$<TARGET_FILE:queue_sender>"
        "$<TARGET_FILE:queue_receiver>"
        "$<TARGET_FILE:fifo_sender>"
        "$<TARGET_FILE:fifo_receiver>"
        "${RESULTS_DIR}"
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
    COMMENT "Running all IPC benchmarks and saving results to results/"
    VERBATIM
)

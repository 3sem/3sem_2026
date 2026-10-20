add_custom_target(run_experiments
    COMMAND ${CMAKE_COMMAND} -E env
        "PROGRAM_A=$<TARGET_FILE:monte_carlo_a>"
        "PROGRAM_B=$<TARGET_FILE:monte_carlo_b>"
        bash "${CMAKE_CURRENT_SOURCE_DIR}/run_experiments.sh"
    DEPENDS
        monte_carlo_a
        monte_carlo_b
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
    USES_TERMINAL
    VERBATIM
)

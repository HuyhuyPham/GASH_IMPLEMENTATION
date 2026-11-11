
# Run a GASH example and check its output.

if(NOT DEFINED GASH_EXECUTABLE OR
   NOT DEFINED GASH_FILE OR
   NOT DEFINED EXPECTED_LINES)
    message(FATAL_ERROR "Missing test arguments")
endif()

execute_process(
    COMMAND "${GASH_EXECUTABLE}" --run "${GASH_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE actual
    ERROR_VARIABLE errors
)

# Support Windows and macOS line endings.
string(REPLACE "\r\n" "\n" actual "${actual}")
string(REGEX REPLACE "[\r\n]+$" "" actual "${actual}")

# Expected lines are separated by |.
string(REPLACE "|" "\n" expected "${EXPECTED_LINES}")

if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR
        "GASH execution failed: ${result}\n${errors}"
    )
endif()

if(NOT "${actual}" STREQUAL "${expected}")
    message(FATAL_ERROR
        "Incorrect output.\n"
        "Expected: [${expected}]\n"
        "Actual: [${actual}]"
    )
endif()

message(STATUS "GASH example passed")
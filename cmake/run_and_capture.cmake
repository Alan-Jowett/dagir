# SPDX-License-Identifier: MIT
# © DagIR Contributors. All rights reserved.
# Runner script for sample tests: execute program and compare output

if(NOT DEFINED PROG)
  message(FATAL_ERROR "PROG not set for run_and_capture.cmake")
endif()

if(NOT DEFINED EXPECTED)
  message(FATAL_ERROR "EXPECTED not set for run_and_capture.cmake")
endif()

if(NOT DEFINED BINARY_OUT)
  message(FATAL_ERROR "BINARY_OUT not set for run_and_capture.cmake")
endif()

# Collect ARG0, ARG1, ... into ARG_LIST (avoid passing semicolons on the command line)
set(ARG_LIST "")
set(_argi 0)
while(DEFINED ARG${_argi})
  list(APPEND ARG_LIST ${ARG${_argi}})
  math(EXPR _argi "${_argi} + 1")
endwhile()

# Run the program and capture stdout/stderr into files
set(_cmdfile "${BINARY_OUT}.cmdline")
file(WRITE "${_cmdfile}" "${PROG}")
foreach(_a IN LISTS ARG_LIST)
  file(APPEND "${_cmdfile}" "\n${_a}")
endforeach()

# Dump how CMake sees the ARG_LIST
execute_process(COMMAND ${CMAKE_COMMAND} -E echo "ARG_LIST=" ${ARG_LIST}
  OUTPUT_FILE "${BINARY_OUT}.argh"
)

# If the provided PROG path doesn't exist (multi-config generators may place
# binaries under ${CMAKE_BINARY_DIR}/Debug or /Release), try some common
# fallback locations before failing.
if(NOT EXISTS "${PROG}")
  get_filename_component(_prog_name ${PROG} NAME)
  set(_candidates
    "${CMAKE_BINARY_DIR}/${_prog_name}"
    "${CMAKE_BINARY_DIR}/Debug/${_prog_name}"
    "${CMAKE_BINARY_DIR}/Release/${_prog_name}"
    "${CMAKE_BINARY_DIR}/$(Configuration)/${_prog_name}"
  )
  set(_found_prog "")
  foreach(_c IN LISTS _candidates)
    if(EXISTS "${_c}")
      set(_found_prog "${_c}")
      break()
    endif()
  endforeach()

  if(_found_prog STREQUAL "")
    message(FATAL_ERROR "Program not found: ${PROG}")
  endif()

  message(STATUS "Using program: ${_found_prog}")
  set(PROG "${_found_prog}")
endif()

execute_process(COMMAND ${PROG} ${ARG_LIST}
  RESULT_VARIABLE run_res
  OUTPUT_FILE "${BINARY_OUT}"
  ERROR_FILE "${BINARY_OUT}.err"
)

if(NOT run_res EQUAL 0)
  message(FATAL_ERROR "Program failed (exit ${run_res}). See ${BINARY_OUT}.err for details.")
endif()

# Compare outputs; if different try a normalized (LF-only) comparison
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${EXPECTED}" "${BINARY_OUT}"
  RESULT_VARIABLE cmp_res
)
message(STATUS "Initial compare_files result: ${cmp_res}")

if(NOT cmp_res EQUAL 0)
  file(READ "${EXPECTED}" _exp_contents)
  string(REPLACE "\r\n" "\n" _exp_norm "${_exp_contents}")
  string(REPLACE "\r" "\n" _exp_norm "${_exp_norm}")
  # Trim trailing whitespace/newlines
  string(REGEX REPLACE "[ \t\n\r]+$" "" _exp_norm "${_exp_norm}")
  set(_exp_tmp "${BINARY_OUT}.expected.normalized")
  file(WRITE "${_exp_tmp}" "${_exp_norm}")

  file(READ "${BINARY_OUT}" _out_contents)
  string(REPLACE "\r\n" "\n" _out_norm "${_out_contents}")
  string(REPLACE "\r" "\n" _out_norm "${_out_norm}")
  # Trim trailing whitespace/newlines
  string(REGEX REPLACE "[ \t\n\r]+$" "" _out_norm "${_out_norm}")
  set(_out_tmp "${BINARY_OUT}.actual.normalized")
  file(WRITE "${_out_tmp}" "${_out_norm}")

  execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${_exp_tmp}" "${_out_tmp}"
    RESULT_VARIABLE cmp_res2)
  message(STATUS "Normalized compare_files result: ${cmp_res2}")

  if(NOT cmp_res2 EQUAL 0)
    message(FATAL_ERROR "Output mismatch. Expected: ${EXPECTED} Got: ${BINARY_OUT}")
  endif()
endif()

# SPDX-License-Identifier: MIT
# Copyright (c) 2025 DagIR contributors

# format.cmake
# Configure-time helper to install a git pre-commit hook that runs clang-format and cppcheck

if(NOT DEFINED GIT_HOOKS_DIR)
  set(GIT_HOOKS_DIR "${CMAKE_SOURCE_DIR}/.git/hooks")
endif()

set(PRECOMMIT_SOURCE "${CMAKE_SOURCE_DIR}/scripts/pre-commit")
set(PRECOMMIT_TARGET "${GIT_HOOKS_DIR}/pre-commit")

if(EXISTS "${PRECOMMIT_SOURCE}")
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
      configure_file(${PRECOMMIT_SOURCE} ${PRECOMMIT_TARGET} COPYONLY)
      # Ensure the hook is executable where appropriate. On UNIX-like systems
      # use CMake's `-E chmod`. On Windows we skip setting executable bits.
      if(UNIX)
        execute_process(COMMAND ${CMAKE_COMMAND} -E chmod +x ${PRECOMMIT_TARGET})
      endif()
      message(STATUS "Installed pre-commit hook to ${PRECOMMIT_TARGET}")
  else()
    message(STATUS "Not a git repository; skipping pre-commit installation")
  endif()
else()
  message(WARNING "Pre-commit template ${PRECOMMIT_SOURCE} not found")
endif()

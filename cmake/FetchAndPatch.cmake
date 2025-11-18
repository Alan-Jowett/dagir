# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: Copyright (c) 2025 Alan Jowett

# Set policy to allow FetchContent_Populate until we migrate to modern approach
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

#[=======================================================================[.rst:
FetchAndPatch
-------------

This module provides a function to fetch external projects using FetchContent
and automatically apply patches before making the project available.

.. command:: fetch_and_patch

  Fetches an external project and applies patches::

    fetch_and_patch(
        <project_name>
        GIT_REPOSITORY <repository_url>
        {GIT_TAG <tag_or_commit> | GIT_BRANCH <branch_name>}
        [PATCHES_DIR <patches_directory>]
        [PATCH_STRIP_LEVEL <strip_level>]
    )

  ``project_name``
    Name of the project to fetch. This will be used as the FetchContent project name.

  ``GIT_REPOSITORY``
    The Git repository URL to clone from.

  ``GIT_TAG``
    The Git tag or commit hash to checkout. Mutually exclusive with GIT_BRANCH.

  ``GIT_BRANCH``
    The Git branch to checkout. Mutually exclusive with GIT_TAG.

  ``PATCHES_DIR``
    Optional directory containing patch files. Defaults to patches/<project_name_lower>.
    Patch files should be named with numeric prefixes (e.g., 0001-fix.patch) for ordering.

  ``PATCH_STRIP_LEVEL``
    Optional strip level for patch application. Defaults to 1 (removes one directory level).

The function will:
1. Declare and fetch the project using FetchContent
2. Find all .patch files in the patches directory
3. Apply patches in sorted order using git apply
4. Make the project available for use

Example usage::

    # Using a Git tag
    fetch_and_patch(DecisionDiagrams
        GIT_REPOSITORY https://github.com/MichalMrena/DecisionDiagrams.git
        GIT_TAG v4.1.0
        PATCHES_DIR ${CMAKE_SOURCE_DIR}/patches/teddy
    )

    # Using a Git branch
    fetch_and_patch(SomeProject
        GIT_REPOSITORY https://github.com/user/project.git
        GIT_BRANCH main
        PATCHES_DIR ${CMAKE_SOURCE_DIR}/patches/project
    )

#]=======================================================================]

include(FetchContent)

function(fetch_and_patch project_name)
    # Parse arguments
    set(options "")
    set(oneValueArgs GIT_REPOSITORY GIT_TAG GIT_BRANCH PATCHES_DIR PATCH_STRIP_LEVEL)
    set(multiValueArgs "")

    cmake_parse_arguments(FAP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT FAP_GIT_REPOSITORY)
        message(FATAL_ERROR "fetch_and_patch: GIT_REPOSITORY is required")
    endif()

    # Validate that either GIT_TAG or GIT_BRANCH is provided, but not both
    if(NOT FAP_GIT_TAG AND NOT FAP_GIT_BRANCH)
        message(FATAL_ERROR "fetch_and_patch: Either GIT_TAG or GIT_BRANCH must be specified")
    endif()

    if(FAP_GIT_TAG AND FAP_GIT_BRANCH)
        message(FATAL_ERROR "fetch_and_patch: GIT_TAG and GIT_BRANCH are mutually exclusive. Use only one.")
    endif()

    # Determine the git reference to use
    if(FAP_GIT_TAG)
        set(git_ref "${FAP_GIT_TAG}")
        set(ref_type "tag/commit")
    else()
        set(git_ref "${FAP_GIT_BRANCH}")
        set(ref_type "branch")
    endif()

    # Set default values
    if(NOT FAP_PATCHES_DIR)
        string(TOLOWER "${project_name}" project_name_lower)
        set(FAP_PATCHES_DIR "${CMAKE_SOURCE_DIR}/patches/${project_name_lower}")
    endif()

    if(NOT FAP_PATCH_STRIP_LEVEL)
        set(FAP_PATCH_STRIP_LEVEL 1)
    endif()

    message(STATUS "Fetching project: ${project_name}")
    message(STATUS "  Repository: ${FAP_GIT_REPOSITORY}")
    message(STATUS "  ${ref_type}: ${git_ref}")
    message(STATUS "  Patches Dir: ${FAP_PATCHES_DIR}")

    # Declare the project with FetchContent
    if(FAP_GIT_TAG)
        FetchContent_Declare(
            ${project_name}
            GIT_REPOSITORY ${FAP_GIT_REPOSITORY}
            GIT_TAG ${FAP_GIT_TAG}
        )
    else()
        FetchContent_Declare(
            ${project_name}
            GIT_REPOSITORY ${FAP_GIT_REPOSITORY}
            GIT_TAG ${FAP_GIT_BRANCH}
        )
    endif()

    # Get the source directory for the project before making available
    FetchContent_GetProperties(${project_name})
    string(TOLOWER "${project_name}" project_name_lower)
    set(project_source_dir "${CMAKE_BINARY_DIR}/_deps/${project_name_lower}-src")

    # Check if already populated
    if(NOT ${project_name_lower}_POPULATED)
        # Populate (download) the project first
        FetchContent_Populate(${project_name})

        # Apply patches if patches directory exists
        if(EXISTS "${FAP_PATCHES_DIR}")
            _apply_patches_to_project("${project_name}" "${project_source_dir}" "${FAP_PATCHES_DIR}" "${FAP_PATCH_STRIP_LEVEL}")
        else()
            message(STATUS "No patches directory found: ${FAP_PATCHES_DIR}")
        endif()

        # Mark as populated to avoid FetchContent_MakeAvailable trying to re-download
        set(${project_name_lower}_POPULATED TRUE)
        set(${project_name_lower}_SOURCE_DIR ${project_source_dir})
        set(${project_name_lower}_BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/${project_name_lower}-build)

        # Add the subdirectory to include the project's CMakeLists.txt
        add_subdirectory(${project_source_dir} ${CMAKE_BINARY_DIR}/_deps/${project_name_lower}-build)
    else()
        message(STATUS "Project ${project_name} already populated")
    endif()

    message(STATUS "Project ${project_name} fetched and integrated successfully")
endfunction()

# Internal function to apply patches to a project
function(_apply_patches_to_project project_name source_dir patches_dir strip_level)
    # Find all patch files in the directory
    file(GLOB patch_files "${patches_dir}/*.patch")
    list(SORT patch_files)

    if(NOT patch_files)
        message(STATUS "No patch files found in ${patches_dir}")
        return()
    endif()

    list(LENGTH patch_files patch_count)
    message(STATUS "Found ${patch_count} patches for ${project_name}")

    # Find git executable
    find_program(GIT_EXECUTABLE git)
    if(NOT GIT_EXECUTABLE)
        message(FATAL_ERROR "Git executable not found. Cannot apply patches to ${project_name}")
    endif()

    # Apply each patch file
    foreach(patch_file ${patch_files})
        get_filename_component(patch_name "${patch_file}" NAME)

        # Check if patch can be applied (not already applied)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} apply --check -p${strip_level} "${patch_file}"
            WORKING_DIRECTORY "${source_dir}"
            RESULT_VARIABLE patch_check_result
            OUTPUT_QUIET
            ERROR_QUIET
        )

        if(patch_check_result EQUAL 0)
            # Patch can be applied
            message(STATUS "  Applying patch: ${patch_name}")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} apply -p${strip_level} "${patch_file}"
                WORKING_DIRECTORY "${source_dir}"
                RESULT_VARIABLE patch_result
                OUTPUT_VARIABLE patch_output
                ERROR_VARIABLE patch_error
            )

            if(patch_result EQUAL 0)
                message(STATUS "    ✓ ${patch_name} applied successfully")
            else()
                # Try with --ignore-whitespace for robustness
                execute_process(
                    COMMAND ${GIT_EXECUTABLE} apply --ignore-whitespace -p${strip_level} "${patch_file}"
                    WORKING_DIRECTORY "${source_dir}"
                    RESULT_VARIABLE patch_retry_result
                    OUTPUT_VARIABLE patch_retry_output
                    ERROR_VARIABLE patch_retry_error
                )

                if(patch_retry_result EQUAL 0)
                    message(STATUS "    ✓ ${patch_name} applied successfully (with whitespace ignore)")
                else()
                    message(WARNING "    ✗ Failed to apply patch ${patch_name}")
                    message(WARNING "       First attempt error: ${patch_error}")
                    message(WARNING "       Retry attempt error: ${patch_retry_error}")
                endif()
            endif()
        else()
            # Patch cannot be applied, likely already applied or conflicts
            message(STATUS "    ○ ${patch_name} already applied or not applicable")
        endif()
    endforeach()
endfunction()

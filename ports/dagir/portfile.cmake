include(vcpkg_common_functions)

# Fetch source from GitHub. Update REF to a released tag when publishing.
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO Alan-Jowett/dagir
  REF v0.1.0
  SHA512 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
)

# Configure CMake to install headers only; disable tests/samples so the build is minimal.
vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH}
  PREFER_NINJA
  OPTIONS
    -DDAGIR_BUILD_TESTS=OFF
    -DDAGIR_SAMPLES=OFF
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_INSTALL_INCLUDEDIR=include
)

vcpkg_install_cmake()

# Ensure package config is present so consumers can `find_package(DagIR CONFIG)`
set(_targets_file "${CURRENT_PACKAGES_DIR}/lib/cmake/DagIR/DagIRTargets.cmake")
if(NOT EXISTS "${_targets_file}")
  # Create a minimal targets file that defines an INTERFACE library
  file(WRITE "${CURRENT_PACKAGES_DIR}/lib/cmake/DagIR/DagIRTargets.cmake"
    "add_library(dagir::dagir INTERFACE)\n"
    "add_library(dagir INTERFACE)\n"
    "target_include_directories(dagir INTERFACE \"$<INSTALL_INTERFACE:include>\")\n"
    "add_library(dagir::dagir ALIAS dagir)\n"
  )
endif()

file(WRITE "${CURRENT_PACKAGES_DIR}/lib/cmake/DagIR/DagIRConfig.cmake"
  "include(\"${CMAKE_CURRENT_LIST_DIR}/DagIRTargets.cmake\")\n"
)
include(vcpkg_common_functions)

vcpkg_from_sourceforge(
  OUT_SOURCE_PATH SOURCE_PATH
  SOURCE_NAME dagir
)

# The project is header-only. Install include/ and export a CMake config that
# allows find_package(dagir CONFIG) to work for vcpkg users.

file(INSTALL
  ${CURRENT_PACKAGES_DIR}/..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\INCLUDE
)

vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH}
  PREFER_NINJA
)

vcpkg_install_cmake()

file(WRITE ${CURRENT_PACKAGES_DIR}/share/dagir/dagir-config.cmake "include(\"${CMAKE_CURRENT_LIST_DIR}/../lib/cmake/DagIR/DagIRTargets.cmake\")\n")

include(vcpkg_common_functions)

# Fetch source from GitHub. Update REF and SHA512 to the desired release tag before
# submitting the port upstream into vcpkg.
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO Alan-Jowett/dagir
  REF v0.1.0
  SHA512 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
)

# Configure and install. DagIR is header-only so we disable tests and samples
# to keep the build minimal inside vcpkg.
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

# Provide a minimal CMake package config if upstream did not install one.
set(_targets_file "${CURRENT_PACKAGES_DIR}/lib/cmake/DagIR/DagIRTargets.cmake")
if(NOT EXISTS "${_targets_file}")
  file(WRITE "${_targets_file}"
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

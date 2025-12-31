# CMake integration for target utilities (e.g., Limine)

cmake_minimum_required(VERSION 3.20)

set(LIMINE_VERSION "${LIMINE_VERSION}" CACHE STRING "Limine version")
if(NOT LIMINE_VERSION)
  set(LIMINE_VERSION 9.2.0)
endif()
set(LIMINE_SOURCE "https://ghostkernel.org/repository/limine/limine-${LIMINE_VERSION}.tar.gz")

# Host tool checks for the pack flow
find_program(XORRISO_EXECUTABLE xorriso)
if(NOT XORRISO_EXECUTABLE)
  message(FATAL_ERROR "xorriso is required to build the ISO (install package: xorriso)")
endif()
find_program(NASM_EXECUTABLE nasm)
if(NOT NASM_EXECUTABLE)
  message(FATAL_ERROR "nasm is required to build Limine (install package: nasm)")
endif()
find_program(CURL_EXECUTABLE curl)
if(NOT CURL_EXECUTABLE)
  message(FATAL_ERROR "curl is required to fetch Limine (install package: curl)")
endif()

# Where to place limine directory (inside repo's target dir)
set(LIMINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/limine-${LIMINE_VERSION})
set(LIMINE_TAR ${CMAKE_CURRENT_BINARY_DIR}/limine-${LIMINE_VERSION}.tar.gz)
set(LIMINE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DOWNLOAD_LIMINE_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/download-limine.sh)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/download-limine.sh.in ${DOWNLOAD_LIMINE_SCRIPT} @ONLY)

# Build limine binaries if missing
add_custom_command(
  OUTPUT ${LIMINE_DIR}/bin/limine-bios.sys
  BYPRODUCTS ${LIMINE_DIR}/bin/limine-bios-cd.bin ${LIMINE_DIR}/bin/limine-uefi-cd.bin ${LIMINE_DIR}/bin/limine
  COMMAND ${CMAKE_COMMAND} -E echo "Preparing Limine ${LIMINE_VERSION}"
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash ${DOWNLOAD_LIMINE_SCRIPT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL
  COMMENT "Download and build Limine (${LIMINE_VERSION})")

add_custom_target(limine DEPENDS ${LIMINE_DIR}/bin/limine-bios.sys)

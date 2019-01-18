# (c) Copyright 2014-2019 HIPPEROS S.A.
# (c) Copyright 2010-2013 Universite Libre de Bruxelles
# (c) Copyright 2006-2013 MangoGem S.A.
#
# The license and distribution terms for this file may be
# found in the file LICENSE.txt in this distribution.

cmake_minimum_required(VERSION 3.5.1)

set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_C_COMPILER "tulippcc")
set(CMAKE_CXX_COMPILER "tulipp++")

find_package(
    hipperos
    PATHS ${HIPPEROS_DIR}
    REQUIRED
)
find_package(
    drivers
    PATHS ${HIPPEROS_DIR}/libs
    REQUIRED
)
find_package(
    fatfs
    PATHS ${HIPPEROS_DIR}/libs
    REQUIRED
)
# For QEMU builds
get_filename_component(
        HIPPEROS_ROOT
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ABSOLUTE
        CACHE
)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${HIPPEROS_ROOT}/cmake")

project(hipeac C CXX)

# Define application directory
set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Set the binaries/libraries output dir to a special directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Generated source directories (required later by the toolchain).
set(APP_GENERATED_SRC_DIR "${CMAKE_BINARY_DIR}/generated/src")
set(APP_GENERATED_HDR_DIR "${CMAKE_BINARY_DIR}/generated/include")

# C flags added to the compilation command line
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-asynchronous-unwind-tables")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-unwind-tables")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fshort-enums")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

# C++ flags added to the compilation command line
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fshort-enums")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdata-sections")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas") # TODO reput

# Add platform-specific compiler flags to the CMAKE_C_FLAGS variable.
hipperosToolchainConfig()

# Extract information from the kernel specification file.
# Will notably set the following variables:
#  - APP_BUILD_TYPE
#  - APP_TARGET_PLATFORM
hipperosReadKernelOptions("${KERNEL_SPEC_FILE}")

# Find the task set file
set(APP_TASK_SET "taskSet.ats" CACHE STRING "Filename of the task set.")
find_file(
    APP_TASK_SET_FILEPATH
    NAMES ${APP_TASK_SET}
    PATHS ${APP_DIR}
    DOC "Path to the task set."
)

if (NOT APP_TASK_SET_FILEPATH)
    message(FATAL_ERROR "Task set file ${APP_TASK_SET} not found.")
endif()

# Generate config file
configure_file(
    ${APP_DIR}/include/app_config.h.in
    ${APP_GENERATED_HDR_DIR}/app_config.h
    @ONLY
)

# Define application source files
set(APP_SRCS
    ${APP_DIR}/src/main.cpp
    ${APP_DIR}/src/ppm.c
    ${APP_DIR}/src/filters.cpp
)

set(APP_INCLUDE
    ${APP_GENERATED_HDR_DIR}
    ${APP_DIR}/include
    /opt/sthem/target/libtulipp
)



include(modules/GetKernelOption)


getKernelOption("${KERNEL_SPEC_FILE}" "isBoard" IS_BOARD)
if (NOT ${IS_BOARD})
    # Create SD/MMC card image
    include(modules/QemuUtils)
    set(SD_IMAGE "${CMAKE_BINARY_DIR}/sd.img")
    addSdImageTarget("sdImage" "${SD_IMAGE}" "${APP_DIR}/files" 16000)
else()
    # Configure a deploy-time script to copy test files on physical boards
    set(DEPLOY_SCRIPT_DIR ${CMAKE_BINARY_DIR}/deploy)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/copyDemoFiles.sh.in
        ${DEPLOY_SCRIPT_DIR}/prerun.sh
        @ONLY
    )
endif()

# Create project main target
add_executable(${PROJECT_NAME} ${APP_SRCS})

add_library(htulipp STATIC IMPORTED)
set_target_properties(htulipp PROPERTIES
   IMPORTED_LOCATION "/opt/sthem/target/libtulipp/libhtulipp.a"
   INTERFACE_LINK_LIBRARIES "hipperos::api"
)

target_link_libraries(
    ${PROJECT_NAME}
        hipperos::api
        hipperos::fatfs
        hipperos::drivers
        htulipp
)

hipperosTaskConfig("${PROJECT_NAME}")

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${APP_INCLUDE}
)

# 1. Generate the init task table (based on your task set file)
# 2. Add platform specific targets
#   - generation of U-boot bootable images
#   - deploy target (to send the U-boot image to the board)
#   - debug target
hipperosTaskSetConfig(
    "${PROJECT_NAME}"
    "${APP_GENERATED_SRC_DIR}"
    "${APP_TASK_SET_FILEPATH}"
)

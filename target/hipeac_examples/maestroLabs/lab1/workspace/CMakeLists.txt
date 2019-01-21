# (c) Copyright 2014-2019 HIPPEROS S.A.
# (c) Copyright 2010-2013 Universite Libre de Bruxelles
# (c) Copyright 2006-2013 MangoGem S.A.
#
# The license and distribution terms for this file may be
# found in the file LICENSE.txt in this distribution.

set(HIPPEROS_DIR "$ENV{HIPPEROS_DIR}")

set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(APP_SCRIPT_DIR ${APP_DIR}/scripts)

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
find_package(
    sds_lib
    PATHS ${HIPPEROS_DIR}/libs
    REQUIRED
)

if(APP_OPENMP)
    find_package(
        homp
        PATHS ${HIPPEROS_DIR}/libs
        REQUIRED
    )
endif()

addOption("TARGET_BOARD_VARIANT" "Variant of the Zynq UltraScale+ processor" "ZU4EV" "ZU3EG;ZU4EV")

if("${TARGET_BOARD_VARIANT}" STREQUAL "ZU4EV")
    set(
        OPENOCD_BOARD_FILE
        "te0820-zu4ev.cfg"
        CACHE
        STRING
        "Correct board file for OpenOCD on ZU4EV"
    )
    set(APP_SDSCC_DIRPOTFIX "4EV")
elseif("${TARGET_BOARD_VARIANT}" STREQUAL "ZU3EG")
    set(APP_SDSCC_DIRPOTFIX "3EG")
endif()
set(SDSCC_PLATFORM "$ENV{HOME}/maestro_platform_files/TE0820_${APP_SDSCC_DIRPOTFIX}/TE0820_zusys_SDSoC")

project(tulippTutorial C CXX)
cmake_minimum_required(VERSION 3.5.1)

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
if(APP_OPENMP)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp=libomp")
endif()

# C++ flags added to the compilation command line
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fshort-enums")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdata-sections")
if(APP_OPENMP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp=libomp")
endif()

if("${HIPPEROS_TOOLCHAIN}" STREQUAL "SDSCC")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
endif()

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

if(APP_OPENMP)
    list(APPEND APP_SRCS
        "${APP_DIR}/src/sepia_omp.cpp"
    )
endif()

set(APP_INCLUDE
    ${APP_GENERATED_HDR_DIR}
    ${APP_DIR}/include
)

# Hook to load the bitstream into the fpga before deploying Maestro
set(DEPLOY_HOOK_PATH "${APP_SCRIPT_DIR}/uBootLoadBitstream.py")


include(modules/GetKernelOption)
getKernelOption("${KERNEL_SPEC_FILE}" "isBoard" IS_BOARD)
if (NOT ${IS_BOARD})
    # For QEMU builds
    get_filename_component(
        HIPPEROS_ROOT
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ABSOLUTE
        CACHE
    )
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${HIPPEROS_ROOT}/cmake")
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
target_link_libraries(
    ${PROJECT_NAME}
        hipperos::api
        hipperos::fatfs
        hipperos::drivers
        hipperos::sds_lib
)
if(APP_OPENMP)
    target_link_libraries(
        ${PROJECT_NAME}
            hipperos::omp
    )
endif()

hipperosTaskConfig("${PROJECT_NAME}")

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${APP_INCLUDE}
)

if("${HIPPEROS_TOOLCHAIN}" STREQUAL "SDSCC")
    include(modules/Sdscc)
    sdscc_enable()
    sdscc_accel("${PROJECT_NAME}" "${APP_DIR}/src/filters.cpp" "hwPassthrough" "0")
    sdscc_accel("${PROJECT_NAME}" "${APP_DIR}/src/filters.cpp" "hwSepia" "0")
    sdscc_accel("${PROJECT_NAME}" "${APP_DIR}/src/filters.cpp" "hwColorConversionRgbxToGray" "0")
    sdscc_accel("${PROJECT_NAME}" "${APP_DIR}/src/filters.cpp" "hwSobelX" "0")
    sdscc_accel("${PROJECT_NAME}" "${APP_DIR}/src/filters.cpp" "hwSobelY" "0")
    sdscc_accel("${PROJECT_NAME}" "${APP_DIR}/src/filters.cpp" "hwScharrX" "0")
    sdscc_accel("${PROJECT_NAME}" "${APP_DIR}/src/filters.cpp" "hwScharrY" "0")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND vivado -mode tcl -source "${CMAKE_CURRENT_SOURCE_DIR}/scripts/genBitstreamBin.tcl"
    )
endif()

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


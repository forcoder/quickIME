# QuickInput Installer Build Script
# Standalone CMake script for building the installer with all dependencies

cmake_minimum_required(VERSION 3.20)

# Project configuration
project(QuickInput_Installer_Build)

# Detect system architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SYSTEM_ARCH "x64")
else()
    set(SYSTEM_ARCH "x86")
endif()

# Detect Visual Studio version
if(MSVC)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.30)
        set(VISUAL_STUDIO_VERSION "2022")
    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.20)
        set(VISUAL_STUDIO_VERSION "2019")
    else()
        set(VISUAL_STUDIO_VERSION "2017")
    endif()
endif()

# Find required tools
find_program(CMAKE_EXECUTABLE cmake)
find_program(NINJA_EXECUTABLE ninja)
find_program(ISCC_EXECUTABLE ISCC)

# Configure paths
set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(BUILD_ROOT "${PROJECT_ROOT}/build")
set(INSTALLER_ROOT "${PROJECT_ROOT}/installer")

# Build configuration
set(BUILD_CONFIGS Debug Release)

# Function to build a specific configuration
function(build_configuration config)
    message(STATUS "Building ${config} configuration...")
    set(CONFIG_BUILD_DIR "${BUILD_ROOT}/${config}")
    file(MAKE_DIRECTORY ${CONFIG_BUILD_DIR})

    # Configure the project
    execute_process(
        COMMAND ${CMAKE_EXECUTABLE}
            -G Ninja
            -DCMAKE_BUILD_TYPE=${config}
            -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
            ${PROJECT_ROOT}
        WORKING_DIRECTORY ${CONFIG_BUILD_DIR}
        RESULT_VARIABLE configure_result
        OUTPUT_VARIABLE configure_output
        ERROR_VARIABLE configure_error
    )

    if(configure_result EQUAL 0)
        message(STATUS "Configuration successful for ${config}")
        message(STATUS "Build output: ${configure_output}")

        # Build the project
        execute_process(
            COMMAND ${NINJA_EXECUTABLE}
            WORKING_DIRECTORY ${CONFIG_BUILD_DIR}
            RESULT_VARIABLE build_result
            OUTPUT_VARIABLE build_output
            ERROR_VARIABLE build_error
        )

        if(build_result EQUAL 0)
            message(STATUS "Build successful for ${config}")
            message(STATUS "Build completed in ${CONFIG_BUILD_DIR}")
        else()
            message(WARNING "Build failed for ${config}: ${build_error}")
        endif()
    else()
        message(WARNING "Configuration failed for ${config}: ${configure_error}")
    endif()
endfunction()

# Function to copy build artifacts
function(copy_artifacts config)
    set(ARTIFACTS_DIR "${BUILD_ROOT}/${config}/bin")
    set(RESOURCE_DIR "${PROJECT_ROOT}/resources")
    set(MODELS_DIR "${PROJECT_ROOT}/models")

    # Copy executable and libraries
    file(GLOB_RECURSE BIN_FILES "${ARTIFACTS_DIR}/*.exe" "${ARTIFACTS_DIR}/*.dll")
    foreach(file IN LISTS BIN_FILES)
        get_filename_component(dir ${file} DIRECTORY)
        get_filename_component(name ${file} NAME)
        file(COPY ${file} DESTINATION "${BUILD_ROOT}/release/bin/${name}")
    endforeach()

    # Copy resources
    file(COPY ${RESOURCE_DIR} DESTINATION "${BUILD_ROOT}/release/")
    file(COPY ${MODELS_DIR} DESTINATION "${BUILD_ROOT}/release/")

    message(STATUS "Copied artifacts for ${config} to release directory")
endfunction()

# Main build process
message(STATUS "Starting QuickInput Installer build process...")

# Step 1: Create build directories
file(MAKE_DIRECTORY ${BUILD_ROOT})
file(MAKE_DIRECTORY "${BUILD_ROOT}/release")

# Step 2: Build each configuration
foreach(config IN LISTS BUILD_CONFIGS)
    build_configuration(${config})
    copy_artifacts(${config})
endforeach()

# Step 3: Prepare installer files
message(STATUS "Preparing installer files...")
file(COPY ${INSTALLER_ROOT}/setup.iss DESTINATION ${BUILD_ROOT}/release/)
file(COPY ${INSTALLER_ROOT}/register_ime.bat DESTINATION ${BUILD_ROOT}/release/)
file(COPY ${INSTALLER_ROOT}/unregister_ime.bat DESTINATION ${BUILD_ROOT}/release/)

# Step 4: Validate installer dependencies
message(STATUS "Validating installer dependencies...")
if(NOT ISCC_EXECUTABLE)
    message(FATAL_ERROR "Inno Setup Compiler (ISCC) not found. Please install Inno Setup v6.x")
endif()

# Step 5: Build the installer package
message(STATUS "Building installer package...")
execute_process(
    COMMAND ${ISCC_EXECUTABLE} /Q /O${BUILD_ROOT}/release ${BUILD_ROOT}/release/setup.iss
    RESULT_VARIABLE installer_build_result
    OUTPUT_VARIABLE installer_output
    ERROR_VARIABLE installer_error
)

if(installer_build_result EQUAL 0)
    message(STATUS "Installer built successfully!")
    message(STATUS "Installer location: ${BUILD_ROOT}/release/QuickInput_Setup.exe")
else()
    message(FATAL_ERROR "Failed to build installer: ${installer_error}")
endif()

# Step 6: Generate build report
configure_file(
    "${PROJECT_ROOT}/installer/build_report_template.txt.in"
    "${BUILD_ROOT}/build_report.txt"
    @ONLY
)

# Step 7: Cleanup temporary files
file(REMOVE_RECURSE "${BUILD_ROOT}/Debug" "${BUILD_ROOT}/Release")

message(STATUS "Build process completed successfully!")
message(STATUS "Final installer: ${BUILD_ROOT}/release/QuickInput_Setup.exe")
message(STATUS "Build report: ${BUILD_ROOT}/build_report.txt")
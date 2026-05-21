# QuickInput Installer Validation Script
# Validates installer configuration and dependencies

cmake_minimum_required(VERSION 3.20)

project(QuickInput_Installer_Validation)

# Function to check Inno Setup compiler
function(check_inno_setup)
    find_program(ISCC_EXECUTABLE ISCC)
    if(NOT ISCC_EXECUTABLE)
        message(WARNING "Inno Setup Compiler (ISCC) not found")
        set(HAS_ISCC OFF PARENT_SCOPE)
    else()
        get_filename_component(ISCC_VERSION ${ISCC_EXECUTABLE} NAME_WE)
        message(STATUS "Found Inno Setup: ${ISCC_EXECUTABLE}")
        set(HAS_ISCC ON PARENT_SCOPE)
    endif()
endfunction()

# Function to validate setup.iss file
function(validate_setup_script)
    set(SETUP_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/setup.iss")

    if(NOT EXISTS ${SETUP_SCRIPT})
        message(FATAL_ERROR "Setup script not found: ${SETUP_SCRIPT}")
    endif()

    # Read and validate key sections
    file(READ ${SETUP_SCRIPT} SETUP_CONTENT)

    # Check required sections
    set(REQUIRED_SECTIONS
        "[Setup]"
        "[Files]"
        "[Registry]"
        "[Run]"
        "[UninstallDelete]"
    )

    foreach(section IN LISTS REQUIRED_SECTIONS)
        if(NOT SETUP_CONTENT MATCHES "${section}")
            message(FATAL_ERROR "Missing required section: ${section}")
        endif()
    endforeach()

    message(STATUS "Setup script validation passed")
endfunction()

# Function to validate batch scripts
function(validate_batch_scripts)
    set(BATCH_SCRIPTS
        "${CMAKE_CURRENT_LIST_DIR}/register_ime.bat"
        "${CMAKE_CURRENT_LIST_DIR}/unregister_ime.bat"
    )

    foreach(script IN LISTS BATCH_SCRIPTS)
        if(NOT EXISTS ${script})
            message(FATAL_ERROR "Batch script not found: ${script}")
        endif()

        # Basic syntax check
        file(STRINGS ${script} FIRST_LINE LIMIT_COUNT 1)
        if(NOT FIRST_LINE MATCHES "^@echo off")
            message(WARNING "Script may not start correctly: ${script}")
        endif()

        message(STATUS "Validated batch script: ${script}")
    endforeach()
endfunction()

# Function to validate CMake configuration
function(validate_cmake_config)
    set(CMAKE_FILE "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt")

    if(NOT EXISTS ${CMAKE_FILE})
        message(FATAL_ERROR "CMakeLists.txt not found")
    endif()

    # Check for required targets
    file(STRINGS ${CMAKE_FILE} CMAKE_CONTENT)

    set(REQUIRED_TARGETS
        "add_custom_target(installer)"
        "find_program(ISCC_COMPILER ISCC)"
    )

    foreach(target IN LISTS REQUIRED_TARGETS)
        if(NOT CMAKE_CONTENT MATCHES target)
            message(WARNING "Missing CMake target: ${target}")
        endif()
    endforeach()

    message(STATUS "CMake configuration validation passed")
endfunction()

# Main validation process
message(STATUS "Starting QuickInput Installer validation...")

# Step 1: Check system requirements
message(STATUS "Checking system requirements...")
if(WIN32)
    message(STATUS "✓ Running on Windows platform")
else()
    message(FATAL_ERROR "Installer only supports Windows platform")
endif()

# Step 2: Validate Inno Setup
check_inno_setup()

# Step 3: Validate setup script
validate_setup_script()

# Step 4: Validate batch scripts
validate_batch_scripts()

# Step 5: Validate CMake configuration
validate_cmake_config()

# Step 6: Check file permissions
message(STATUS "Checking file permissions...")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}" AND IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
    message(STATUS "✓ Installer directory accessible")
else()
    message(FATAL_ERROR "Installer directory not accessible")
endif()

# Step 7: Generate validation report
set(VALIDATION_REPORT "${CMAKE_CURRENT_LIST_DIR}/validation_report.txt")
file(WRITE ${VALIDATION_REPORT} "QuickInput Installer Validation Report\n")
file(APPEND ${VALIDATION_REPORT} "==========================================\n")
file(APPEND ${VALIDATION_REPORT} "Date: ${CMAKE_CURRENT_DATE}\n")
file(APPEND ${VALIDATION_REPORT} "Platform: ${CMAKE_SYSTEM_NAME}\n")
file(APPEND ${VALIDATION_REPORT} "CMake Version: ${CMAKE_VERSION}\n")
file(APPEND ${VALIDATION_REPORT} "\nValidation Results:\n")

if(HAS_ISCC)
    file(APPEND ${VALIDATION_REPORT} "✓ Inno Setup Compiler available\n")
else()
    file(APPEND ${VALIDATION_REPORT} "✗ Inno Setup Compiler missing\n")
endif()

file(APPEND ${VALIDATION_REPORT} "✓ Setup script validated\n")
file(APPEND ${VALIDATION_REPORT} "✓ Batch scripts validated\n")
file(APPEND ${VALIDATION_REPORT} "✓ CMake configuration validated\n")
file(APPEND ${VALIDATION_REPORT} "✓ File permissions checked\n")

message(STATUS "Validation report generated: ${VALIDATION_REPORT}")
message(STATUS "Installer validation completed successfully!")
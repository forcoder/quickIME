# QuickInput Installer CMake Configuration
# Configures Inno Setup integration and build process

cmake_minimum_required(VERSION 3.20)
project(QuickInput_Installer)

# Find Inno Setup Compiler (ISCC)
find_program(ISCC_COMPILER ISCC)
if(NOT ISCC_COMPILER)
    message(WARNING "Inno Setup Compiler (ISCC) not found. Please install Inno Setup v6.x")
    set(ISCC_COMPILER "")
else()
    get_filename_component(ISCC_PATH ${ISCC_COMPILER} DIRECTORY)
    set(ISCC_INCLUDE_DIRS "${ISCC_PATH}")
    message(STATUS "Found Inno Setup Compiler: ${ISCC_COMPILER}")
endif()

# Include directories
include_directories(${ISCC_INCLUDE_DIRS})

# Define source files
set(INSTALLER_SOURCES
    setup.iss
    register_ime.bat
    unregister_ime.bat
)

# Custom target for building installer
add_custom_target(installer ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/installer_temp
    COMMAND ${ISCC_COMPILER} /Q /O${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/installer_temp/setup.iss
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${INSTALLER_SOURCES}
    COMMENT "Building QuickInput installer with Inno Setup"
)

# Copy resources to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../README.md DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE DESTINATION ${CMAKE_BINARY_DIR})

# Add custom commands for file management
add_custom_command(TARGET installer PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/installer_temp
    COMMENT "Creating installer temp directory"
)

add_custom_command(TARGET installer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/installer_temp
    COMMENT "Cleaning up installer temp directory"
)

# Install rules
install(FILES ${CMAKE_BINARY_DIR}/QuickInput_Setup.exe
    DESTINATION .
    RENAME QuickInput_Setup.exe
)

# Documentation target
add_custom_target(documentation
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../docs/INSTALLATION.md ${CMAKE_BINARY_DIR}/INSTALLATION.md
    COMMENT "Copying installation documentation"
)

# Validation target
add_custom_target(validate_installer
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/validate_installer.cmake
    COMMENT "Validating installer configuration"
)

# Dependencies
add_dependencies(installer validate_installer)

# Output information
message(STATUS "Installer configuration:")
message(STATUS "  Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "  ISCC compiler: ${ISCC_COMPILER}")

# Create version file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version_info.txt.in
    ${CMAKE_BINARY_DIR}/version_info.txt
    @ONLY
)
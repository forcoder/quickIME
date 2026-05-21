cmake_minimum_required(VERSION 3.20)
project(quickInput VERSION 1.0.0 LANGUAGES CXX)

# 查找 SQLite3
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SQLITE3 sqlite3)
endif()

if(NOT SQLITE3_FOUND)
    find_library(SQLITE3_LIBRARY NAMES sqlite3 sqlite)
    if(SQLITE3_LIBRARY)
        set(SQLITE3_FOUND TRUE)
        get_filename_component(SQLITE3_LIB_DIR ${SQLITE3_LIBRARY} DIRECTORY)
        find_path(SQLITE3_INCLUDE_DIR sqlite3.h PATHS ${SQLITE3_LIB_DIR}/.. ${SQLITE3_LIB_DIR}/../include /usr/include /usr/local/include)
    endif()
endif()

if(SQLITE3_FOUND)
    message(STATUS "Found SQLite3: ${SQLITE3_LIBRARY}")
else()
    message(WARNING "SQLite3 not found. Knowledge base module will not compile.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Options ──
option(QI_BUILD_TESTS    "Build unit tests"       ON)
option(QI_BUILD_INSTALLER "Build installer"       ON)
option(QI_ENABLE_AI      "Enable AI inference"    ON)
option(QI_ENABLE_CLOUD   "Enable cloud LLM API"   OFF)

if(QI_ENABLE_AI)
    # 查找llama.cpp（可选）
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LLAMACPP llama-cpp)
    endif()

    if(NOT LLAMACPP_FOUND)
        message(STATUS "llama.cpp not found, AI module will use mock implementation")
    else()
        message(STATUS "Found llama.cpp: ${LLAMACPP_LIBRARIES}")
    endif()
endif()

# ── Windows SDK / TSF ──
set(CMAKE_SYSTEM_VERSION 10.0.22621.0)

# ── Source groups ──
file(GLOB_RECUR_SOURCES
    src/core/*.cpp
    src/core/test_context.cpp
    src/ime/*.cpp
    src/ui/*.cpp
    src/knowledge/*.cpp
    src/config/*.cpp
    src/utils/*.cpp
    src/cs/*.cpp
)

if(QI_ENABLE_AI)
    # 添加AI模块源文件
    file(GLOB AI_SOURCES "${CMAKE_SOURCE_DIR}/src/ai/*.cpp")
    if(AI_SOURCES)
        list(APPEND _SOURCES ${AI_SOURCES})

        # 创建独立的AI库（可选）
        add_library(quickinput_ai STATIC ${AI_SOURCES})
        target_include_directories(quickinput_ai PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        )
        target_link_libraries(quickinput_ai quickinput_ime)

        # 链接llama.cpp如果找到
        if(LLAMACPP_FOUND)
            target_link_libraries(quickinput_ai ${LLAMACPP_LIBRARIES})
        endif()
    endif()
endif()

# ── Main IME DLL (COM server) ──
add_library(quickinput_ime SHARED
    ${_SOURCES}
    resources/ime.rc
    src/dllmain.cpp
)

target_include_directories(quickinput_ime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(quickinput_ime PRIVATE
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(quickinput_ime PRIVATE
    ole32 oleaut32 uuid
    msctf
    shlwapi
    advapi32
    user32 gdi32
    shell32
    ${SQLITE3_LIBRARY}
)

# ── Config Panel EXE ──
add_executable(quickinput_config
    config_panel/main.cpp
    config_panel/config_ui.cpp
    resources/config.rc
)

target_include_directories(quickinput_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(quickinput_config PRIVATE
    comctl32 ole32
    user32 gdi32 shell32
    ${SQLITE3_LIBRARY}
)

# ── Installer (WiX) ──
if(QI_BUILD_INSTALLER)
    # WiX installer project reference
    add_custom_target(installer
        COMMAND echo "Build installer with WiX toolset"
    )
endif()

# ── Tests ──
if(QI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── AI Module Test ──
if(QI_ENABLE_AI AND QI_BUILD_TESTS)
    add_executable(test_ai src/ai/test_ai.cpp)
    target_link_libraries(test_ai PRIVATE quickinput_ai)
    target_include_directories(test_ai PRIVATE ${CMAKE_SOURCE_DIR}/include)
endif()

# ── Context Module Test ──
if(QI_BUILD_TESTS)
    add_executable(test_context src/core/test_context.cpp)
    target_link_libraries(test_context PRIVATE
        ole32 oleaut32 uuid
        msctf
        shlwapi
        advapi32
        user32 gdi32
        shell32
    )
    target_include_directories(test_context PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# ── Knowledge Base Test ──
if(QI_BUILD_TESTS)
    add_executable(test_kb src/knowledge/test_kb.cpp)
    target_link_libraries(test_kb PRIVATE ${SQLITE3_LIBRARY})
    target_include_directories(test_kb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# ── Install rules ──
install(TARGETS quickinput_ime quickinput_config
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

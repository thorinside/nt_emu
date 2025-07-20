cmake_minimum_required(VERSION 3.16)
project(DistingNTEmulator VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Objective-C++ on macOS
if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
endif()

# Platform-specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
endif()

# Find packages
find_package(PkgConfig REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# PortAudio
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party/imgui
    ${PROJECT_SOURCE_DIR}/third_party/imgui/backends
    ${PROJECT_SOURCE_DIR}/third_party/json/include
    ${PORTAUDIO_INCLUDE_DIRS}
)

# ImGui sources
set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

# GUI emulator sources
set(EMULATOR_SOURCES
    src/main.cpp
    src/core/emulator.cpp
    src/core/plugin_loader.cpp
    src/core/api_shim.cpp
    src/core/audio_engine.cpp
    src/core/audio_device_manager.cpp
    src/core/midi_handler.cpp
    src/hardware/display.cpp
    src/hardware/hardware_interface.cpp
    src/hardware/io_manager.cpp
    src/ui/main_window.cpp
    src/ui/audio_settings_dialog.cpp
    src/ui/audio_panel.cpp
    src/ui/midi_panel.cpp
    src/ui/plugin_panel.cpp
    src/utils/file_watcher.cpp
    src/utils/logger.cpp
    src/utils/config.cpp
    src/utils/file_dialog.mm
)

# Create executable
add_executable(${PROJECT_NAME}
    ${EMULATOR_SOURCES}
    ${IMGUI_SOURCES}
)

# Platform-specific definitions
if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __MACOSX_CORE__
    )
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_DL_LIBS}
    glfw
    ${OPENGL_LIBRARIES}
)

# Add PortAudio manually to avoid pkg-config issues
target_link_directories(${PROJECT_NAME} PRIVATE ${PORTAUDIO_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} portaudio)

# Platform-specific libraries
if(APPLE)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(COREMIDI_LIBRARY CoreMIDI)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(${PROJECT_NAME}
        ${COREAUDIO_LIBRARY}
        ${COREMIDI_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${COCOA_LIBRARY}
    )
endif()

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic
    ${PORTAUDIO_CFLAGS_OTHER}
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Test plugins
add_subdirectory(test_plugins)
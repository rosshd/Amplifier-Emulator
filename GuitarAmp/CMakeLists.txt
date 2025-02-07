cmake_minimum_required(VERSION 3.15)

# Project name
project(GuitarAmp VERSION 1.0.0)

# Tell CMake where to find JUCE
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/install")

# Add JUCE as a subdirectory
add_subdirectory(JUCE)

# Create the main target
juce_add_gui_app(GuitarAmp
    VERSION "1.0.0"
    PRODUCT_NAME "Guitar Amp"
    COMPANY_NAME "YourCompany"
)

# Generate JuceHeader.h
juce_generate_juce_header(GuitarAmp)

# Add source files
target_sources(GuitarAmp
    PRIVATE
        Source/Main.cpp
)

# Required JUCE modules
target_link_libraries(GuitarAmp
    PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)

# Add required compiler definitions
target_compile_definitions(GuitarAmp
    PRIVATE
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)
cmake_minimum_required(VERSION 3.15)

# Project name and version
project(APComp VERSION 0.0.1)

# Set the path to the JUCE library (relative path)
set(JUCE_DIR ${CMAKE_SOURCE_DIR}/JuceLibraryCode)

# Add JUCE as a subdirectory
add_subdirectory(${JUCE_DIR})

# Create the plugin target
juce_add_plugin(APComp
    # Plugin characteristics
    PLUGIN_MANUFACTURER_CODE APMG
    PLUGIN_CODE APCV
    FORMATS AU VST3
    PRODUCT_NAME "APComp"

    # Source files
    SOURCES
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
	Source/APCommon.cpp
)

# Link libraries
target_link_libraries(APComp
    PRIVATE
        juce::juce_audio_utils
)
cmake_minimum_required(VERSION 3.14)
project(APComp VERSION 0.0.1)

include(FetchContent)

# Download JUCE during the configure step
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(APComp
    PLUGIN_MANUFACTURER_CODE APMG
    PLUGIN_CODE APCV
    FORMATS AU VST3
    PRODUCT_NAME "APComp"
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
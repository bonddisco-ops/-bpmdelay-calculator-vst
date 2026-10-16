cmake_minimum_required(VERSION 3.22)
project(DelayCalc VERSION 1.0.0)
 
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
 
juce_add_plugin(DelayCalc
    COMPANY_NAME "DelayCalc"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Dlyc
    PLUGIN_CODE Dcal
    FORMATS VST3
    COPY_PLUGIN_AFTER_BUILD FALSE
    PRODUCT_NAME "Delay Calculator")
 
target_sources(DelayCalc PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
 
target_compile_definitions(DelayCalc PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
 
target_link_libraries(DelayCalc
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_warning_flags)
 

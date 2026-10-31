cmake_minimum_required(VERSION 3.15)
project(Dosering VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Dosering
    PLUGIN_MANUFACTURER_CODE Phnk
    PLUGIN_CODE Dosr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "DOSERING"
    COMPANY_NAME "Phunkshunz AUDIO"
)

target_sources(Dosering PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(Dosering PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

target_link_libraries(Dosering PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    juce::juce_dsp
)

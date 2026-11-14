cmake_minimum_required(VERSION 3.22)

project(DOSERING VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE hämtas automatiskt vid bygget (eller från JUCE_DIR om den finns lokalt)
if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory(${JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# DOSERING är en ljudeffekt (botten och pump). Linux: bygg även LV2
set(PHZ_EXTRA_FORMATS "")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PHZ_EXTRA_FORMATS LV2)
endif()
juce_add_plugin(Dosering
    COMPANY_NAME "Phunkshunz AUDIO"
    PRODUCT_NAME "DOSERING"
    BUNDLE_ID "com.phunkshunz.dosering"
    PLUGIN_MANUFACTURER_CODE Phnk
    PLUGIN_CODE Dosr
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect
    LV2URI "https://phunkshunz.github.io/lv2/Dosering"
    FORMATS VST3 AU ${PHZ_EXTRA_FORMATS})

target_sources(Dosering
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

target_compile_definitions(Dosering
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

# Inbyggda typsnitt: IBM Plex Sans Condensed + Plex Mono (SIL Open Font License, se fonts/OFL.txt)
file(GLOB MH_FONTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.ttf)
juce_add_binary_data(DoseringData SOURCES ${MH_FONTS})
set_target_properties(DoseringData PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(Dosering
    PRIVATE
        DoseringData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Linux: inget ljudkort och inga X-tillägg behövs för en plugin
if(UNIX AND NOT APPLE AND TARGET Dosering)
    target_compile_definitions(Dosering PUBLIC JUCE_ALSA=0 JUCE_JACK=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0 JUCE_USE_XSHM=0)
endif()

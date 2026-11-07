cmake_minimum_required(VERSION 3.22)

# Fungerar pa gamla Mac-datorer (t.ex. MacBook Pro Late 2013 med Big Sur)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minsta macOS-version")
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal Binary")
endif()

project(KOLDBARAREN VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

set(KOLD_FORMATS VST3)
if(APPLE)
    list(APPEND KOLD_FORMATS AU)
endif()

# Linux: bygg även LV2
set(PHZ_EXTRA_FORMATS "")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PHZ_EXTRA_FORMATS LV2)
endif()
# DEMO: cmake -DKOLD_DEMO=ON ger KOLDBARAREN-DEMO, ett eget plugin som kan ligga bredvid fullversionen
option(KOLD_DEMO "Bygg demoversionen" OFF)
if(KOLD_DEMO)
    set(KOLD_PRODUCT "KOLDBARAREN-DEMO")
    set(KOLD_BUNDLE "com.phunkshunz.koldbararendemo")
    set(KOLD_CODE Kldd)
    set(KOLD_LV2 "https://phunkshunz.github.io/lv2/KoldbararenDemo")
else()
    set(KOLD_PRODUCT "KOLDBARAREN")
    set(KOLD_BUNDLE "com.phunkshunz.koldbararen")
    set(KOLD_CODE Kldb)
    set(KOLD_LV2 "https://phunkshunz.github.io/lv2/Koldbararen")
endif()

juce_add_plugin(Koldbararen
    COMPANY_NAME                "Phunkshunz AUDIO"
    COMPANY_WEBSITE             "https://phunkshunz.github.io/maskinhallen"
    BUNDLE_ID                   "${KOLD_BUNDLE}"
    PLUGIN_MANUFACTURER_CODE    Phnk
    PLUGIN_CODE                 ${KOLD_CODE}
    PRODUCT_NAME                "${KOLD_PRODUCT}"
    DESCRIPTION                 "MASKINHALLEN - the bottom machine: drawn duck, transient, phase-locked sub, sub distortion"
    LV2URI "${KOLD_LV2}"
    FORMATS                     ${KOLD_FORMATS} ${PHZ_EXTRA_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Dynamics
    AU_MAIN_TYPE                kAudioUnitType_Effect
)

target_sources(Koldbararen PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

if(KOLD_DEMO)
    target_compile_definitions(Koldbararen PUBLIC KOLD_DEMO=1)
endif()

target_compile_definitions(Koldbararen PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_compile_options(Koldbararen PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(Koldbararen
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

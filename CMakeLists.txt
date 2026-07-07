cmake_minimum_required(VERSION 3.15)

project(GranularFX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Fetch JUCE automatically -------------------------------------------
# This downloads JUCE the first time you configure the project (needs
# internet access once). If you'd rather use a local JUCE checkout,
# comment this block out and use add_subdirectory(path/to/JUCE) instead.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
)
FetchContent_MakeAvailable(JUCE)

# --- Plugin target -------------------------------------------------------
# AU only makes sense on macOS (for Logic/GarageBand); Windows just gets VST3.
if (APPLE)
    set (GRANULARIX_FORMATS VST3 AU)
else()
    set (GRANULARIX_FORMATS VST3)
endif()

juce_add_plugin(GranularFX
    COMPANY_NAME "pxiqzes"
    PLUGIN_MANUFACTURER_CODE Pxzs
    PLUGIN_CODE Grnl
    FORMATS ${GRANULARIX_FORMATS}
    PRODUCT_NAME "GRANULARIX"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Generates JuceHeader.h, which PluginProcessor.h/PluginEditor.h include
juce_generate_juce_header(GranularFX)

target_sources(GranularFX PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(GranularFX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(GranularFX PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ==============================================================================
#  Mugen Space Drum — configuração de compilação (CMake)
#  Made by Mugen
#
#  O JUCE é descarregado automaticamente da fonte oficial
#  (github.com/juce-framework/JUCE) numa versão fixa, para que todas as
#  compilações usem exatamente o mesmo código.
# ==============================================================================

cmake_minimum_required(VERSION 3.22)

# Apple Silicon por defeito (tem de ser definido antes de project()).
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Arquitetura macOS")
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Versão mínima do macOS")

project(MugenSpaceDrum VERSION 0.6.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.14
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MugenSpaceDrum
    COMPANY_NAME                "Mugen"
    BUNDLE_ID                   "com.mugen.spacedrum"
    PRODUCT_NAME                "Mugen Space Drum"
    VERSION                     ${PROJECT_VERSION}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Mugn
    PLUGIN_CODE                 Msdr
    VST3_CATEGORIES             Instrument Synth
    FORMATS                     VST3
)

target_sources(MugenSpaceDrum PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/SpaceDrumVoice.cpp
    Source/AmbienceEngine.cpp
    Source/SampleBank.cpp
    Source/ReverbEngine.cpp
    Source/MotionEngine.cpp
    Source/ScaleEngine.cpp
    Source/PresetManager.cpp
    Source/OrbVisualizer.cpp
)

target_compile_definitions(MugenSpaceDrum PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MugenSpaceDrum
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

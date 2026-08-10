cmake_minimum_required(VERSION 3.22)

project(VRCScriabinNoteLink VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    # Exact commit for the upstream 8.0.6 tag; do not replace with a floating branch.
    GIT_TAG 51a8a6d7aeae7326956d747737ccf1575e61e209
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(VRCScriabinNoteLink
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "asimosound"
    BUNDLE_ID "com.asimosound.vrcscriabinnote"
    PLUGIN_MANUFACTURER_CODE Asim
    PLUGIN_CODE VsNL
    FORMATS VST3 Standalone
    PRODUCT_NAME "VRC Scriabin Note Link"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(VRCScriabinNoteLink)

target_sources(VRCScriabinNoteLink PRIVATE
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginProcessor.h
    Source/Plugin/PluginEditor.cpp
    Source/Plugin/PluginEditor.h
    Source/Core/DetectionMailbox.h
    Source/Core/FundamentalDetector.h
    Source/Core/NoteStabilizer.h
    Source/Osc/SenderOwnership.cpp
    Source/Osc/SenderOwnership.h
    Source/Osc/VrchatOscSender.cpp
    Source/Osc/VrchatOscSender.h
)

target_include_directories(VRCScriabinNoteLink PRIVATE Source)

target_compile_definitions(VRCScriabinNoteLink
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(VRCScriabinNoteLink
    PRIVATE
        juce::juce_audio_utils
        juce::juce_osc
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

include(CTest)
if(BUILD_TESTING)
    add_executable(vrc_note_link_core_tests tests/CoreTests.cpp)
    target_include_directories(vrc_note_link_core_tests PRIVATE Source)
    target_compile_features(vrc_note_link_core_tests PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(vrc_note_link_core_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(vrc_note_link_core_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME core_contracts COMMAND vrc_note_link_core_tests)

    add_executable(vrc_note_link_osc_tests
        tests/OscTests.cpp
        Source/Osc/SenderOwnership.cpp
        Source/Osc/VrchatOscSender.cpp
    )
    target_include_directories(vrc_note_link_osc_tests PRIVATE Source)
    target_compile_definitions(vrc_note_link_osc_tests PRIVATE
        VRCNL_TESTING=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
    target_link_libraries(vrc_note_link_osc_tests PRIVATE
        juce::juce_audio_utils
        juce::juce_osc
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
    add_test(NAME osc_contracts COMMAND vrc_note_link_osc_tests)

    add_executable(vrc_note_link_processor_tests tests/ProcessorTests.cpp)
    target_include_directories(vrc_note_link_processor_tests PRIVATE
        Source
        "${CMAKE_CURRENT_BINARY_DIR}/VRCScriabinNoteLink_artefacts/JuceLibraryCode"
    )
    target_link_libraries(vrc_note_link_processor_tests PRIVATE
        VRCScriabinNoteLink
        juce::juce_audio_utils
        juce::juce_osc
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
    add_test(NAME processor_contracts COMMAND vrc_note_link_processor_tests)
endif()

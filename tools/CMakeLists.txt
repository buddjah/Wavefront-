# Outils de développement (non inclus dans le build par défaut).
# Activer avec -DWAVEFRONT_BUILD_TOOLS=ON.

juce_add_console_app(wavefront_render_ui PRODUCT_NAME wavefront_render_ui)
juce_generate_juce_header(wavefront_render_ui)

target_sources(wavefront_render_ui
    PRIVATE
        render_ui.cpp
        ${CMAKE_SOURCE_DIR}/Source/PluginProcessor.cpp
        ${CMAKE_SOURCE_DIR}/Source/PluginEditor.cpp
        ${CMAKE_SOURCE_DIR}/Source/Params/Parameters.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/DopplerEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/Effects/EffectsChain.cpp
        ${CMAKE_SOURCE_DIR}/Source/Presets/PresetManager.cpp
        ${CMAKE_SOURCE_DIR}/Source/UI/PathEditor/PathEditorComponent.cpp
        ${CMAKE_SOURCE_DIR}/Source/UI/LookAndFeel/WavefrontLookAndFeel.cpp)

target_include_directories(wavefront_render_ui PRIVATE ${CMAKE_SOURCE_DIR}/Source)

target_compile_definitions(wavefront_render_ui
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
        JucePlugin_Name="Wavefront")

target_link_libraries(wavefront_render_ui
    PRIVATE
        WavefrontData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Validation headless du plugin (torture test façon pluginval) ---
juce_add_console_app(wavefront_validate PRODUCT_NAME wavefront_validate)
juce_generate_juce_header(wavefront_validate)
target_sources(wavefront_validate
    PRIVATE
        validate.cpp
        ${CMAKE_SOURCE_DIR}/Source/PluginProcessor.cpp
        ${CMAKE_SOURCE_DIR}/Source/PluginEditor.cpp
        ${CMAKE_SOURCE_DIR}/Source/Params/Parameters.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/DopplerEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/Effects/EffectsChain.cpp
        ${CMAKE_SOURCE_DIR}/Source/Presets/PresetManager.cpp
        ${CMAKE_SOURCE_DIR}/Source/UI/PathEditor/PathEditorComponent.cpp
        ${CMAKE_SOURCE_DIR}/Source/UI/LookAndFeel/WavefrontLookAndFeel.cpp)
target_include_directories(wavefront_validate PRIVATE ${CMAKE_SOURCE_DIR}/Source)
target_compile_definitions(wavefront_validate
    PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1
            JucePlugin_Name="Wavefront")
target_link_libraries(wavefront_validate
    PRIVATE
        WavefrontData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
add_test(NAME wavefront_plugin_validation COMMAND wavefront_validate)

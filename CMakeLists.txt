cmake_minimum_required(VERSION 3.22)

# ============================================================================
#  Wavefront — VST3 Doppler / warping effect (JUCE, C++17)
#  Phase 0 : squelette compilable, plugin VST3 vide.
#
#  Cible principale du projet : Windows / VST3 uniquement.
#  Le code DSP (Source/DSP/) doit rester indépendant de la plateforme afin
#  de ne pas fermer la porte à un portage macOS ultérieur.
# ============================================================================

project(Wavefront VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Toujours générer compile_commands.json (utile pour les IDE / clangd).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sur les générateurs mono-config (Makefiles, Ninja), défaut = Release.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ----------------------------------------------------------------------------
#  JUCE
#
#  Choix : FetchContent plutôt qu'un submodule Git.
#  Justification :
#    - la version de JUCE est épinglée par un tag (GIT_TAG) directement dans
#      ce fichier : la source de vérité est le CMakeLists, pas l'état d'un
#      submodule que l'on peut oublier de mettre à jour ;
#    - `git clone` du dépôt reste léger et reproductible (pas de `submodule
#      update --init --recursive` à ne pas oublier) ;
#    - la mise à jour de JUCE = changer une seule ligne (le tag) et
#      reconfigurer.
#    - GIT_SHALLOW ON évite de rapatrier tout l'historique de JUCE.
# ----------------------------------------------------------------------------
include(FetchContent)

set(WAVEFRONT_JUCE_TAG "8.0.4" CACHE STRING "Tag JUCE à récupérer")

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${WAVEFRONT_JUCE_TAG}
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(juce)

# ----------------------------------------------------------------------------
#  Cible du plugin — VST3 uniquement (pas de Standalone / AU / AAX).
# ----------------------------------------------------------------------------
juce_add_plugin(Wavefront
    PRODUCT_NAME              "Wavefront"
    COMPANY_NAME              "Wavefront"
    BUNDLE_ID                 com.wavefront.wavefront
    PLUGIN_MANUFACTURER_CODE  Wvft
    PLUGIN_CODE               Wvf1
    FORMATS                   VST3
    VST3_CATEGORIES           Fx
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
)

# Génère le header JuceHeader.h et les définitions de config du module.
juce_generate_juce_header(Wavefront)

target_sources(Wavefront
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h

        Source/Params/Parameters.cpp
        Source/Params/Parameters.h
        Source/Params/ParameterIDs.h

        Source/DSP/HQFractionalDelay.h
        Source/DSP/PathModel.h
        Source/DSP/DopplerEngine.cpp
        Source/DSP/DopplerEngine.h

        Source/DSP/Granular/GranularEngine.h
        Source/DSP/Modulation/LFO.h
        Source/DSP/Modulation/ModMatrix.h

        Source/UI/PathEditor/PathEditorComponent.cpp
        Source/UI/PathEditor/PathEditorComponent.h

        Source/UI/LookAndFeel/WavefrontLookAndFeel.cpp
        Source/UI/LookAndFeel/WavefrontLookAndFeel.h
        Source/UI/LookAndFeel/WavefrontColours.h
)

target_compile_definitions(Wavefront
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(Wavefront
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(Wavefront
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# ----------------------------------------------------------------------------
#  Tests (CTest) — cible prête à recevoir les futurs tests DSP.
# ----------------------------------------------------------------------------
enable_testing()
add_subdirectory(Tests)

# ----------------------------------------------------------------------------
#  Outils de développement (optionnels) — ex. rendu de l'UI en PNG.
# ----------------------------------------------------------------------------
option(WAVEFRONT_BUILD_TOOLS "Construire les outils de dev (rendu UI, etc.)" OFF)
if(WAVEFRONT_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

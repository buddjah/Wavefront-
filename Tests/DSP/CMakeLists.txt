# Tests DSP — Phase 0.
#
# Une seule cible "sanity" pour valider que le pipeline CTest fonctionne.
# Les phases suivantes ajouteront ici les tests unitaires du moteur
# Doppler/warping, granulaire, matrice de modulation, etc.
#
# Ces tests restent volontairement indépendants de JUCE et de la plateforme
# pour pouvoir tourner en CI (y compris sur un runner Linux) sans dépendance UI.

add_executable(wavefront_dsp_tests
    SanityTest.cpp
)
target_compile_features(wavefront_dsp_tests PRIVATE cxx_std_17)
add_test(NAME wavefront_dsp_sanity COMMAND wavefront_dsp_tests)

# Test de la ligne à retard (header-only, indépendant de JUCE).
add_executable(wavefront_delayline_tests
    DelayLineTest.cpp
)
target_compile_features(wavefront_delayline_tests PRIVATE cxx_std_17)
target_include_directories(wavefront_delayline_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/Source)
add_test(NAME wavefront_delayline COMMAND wavefront_delayline_tests)

# Test d'intégration DSP (rend de l'audio via les moteurs). Nécessite JUCE :
# on en fait une console app JUCE liée à juce_dsp / juce_graphics.
juce_add_console_app(wavefront_engine_tests PRODUCT_NAME wavefront_engine_tests)
juce_generate_juce_header(wavefront_engine_tests)
target_sources(wavefront_engine_tests
    PRIVATE
        EngineIntegrationTest.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/DopplerEngine.cpp)
target_include_directories(wavefront_engine_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/Source)
target_compile_definitions(wavefront_engine_tests
    PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(wavefront_engine_tests
    PRIVATE
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
add_test(NAME wavefront_engine COMMAND wavefront_engine_tests)

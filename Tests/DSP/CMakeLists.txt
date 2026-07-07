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

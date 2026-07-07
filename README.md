# Wavefront

Plugin **VST3** — effet **Doppler / warping** haute qualité, avec moteur
granulaire, matrice de modulation (4 LFOs), chaîne d'effets post et **Path
Editor 2D**. Projet personnel / portfolio.

- **Format** : VST3 uniquement
- **Plateforme cible** : Windows
- **Langage / framework** : C++17, [JUCE](https://juce.com) 8.x
- **Build** : CMake (JUCE récupéré via `FetchContent`)

> Le code DSP (`Source/DSP/`) est maintenu **indépendant de la plateforme**
> pour ne pas fermer la porte à un portage macOS ultérieur.

## État d'avancement

| Phase | Contenu | Statut |
|---|---|---|
| **0** | Init dépôt, squelette JUCE/CMake, VST3 vide compilable | ✅ en cours |
| 1 | Moteur Doppler/warping HQ | à venir |
| 2 | Moteur granulaire + matrice de modulation | à venir |
| 3 | UI du Path Editor 2D | à venir |
| 4 | Direction artistique néon / LookAndFeel | à venir |
| 5 | Sampler, presets, polish, validation `pluginval` | à venir |

Voir [`docs/PHASES.md`](docs/PHASES.md) pour le détail.

## Compilation

Prérequis : CMake ≥ 3.22, un compilateur C++17 (MSVC sur Windows), et l'accès
réseau pour que `FetchContent` récupère JUCE au premier configure.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Le `.vst3` généré se trouve dans l'arborescence de build de JUCE
(`build/Wavefront_artefacts/Release/VST3/`).

### Tests

```bash
ctest --test-dir build --output-on-failure
```

## Arborescence

```
Source/
  DSP/{Granular,Modulation,Effects,Sampler}   moteurs DSP (indépendants plateforme)
  UI/{LookAndFeel,PathEditor,Meters,Knobs,     composants d'interface
      Panels,ModMatrixView,PresetBrowser}
  Params/                                       gestion des paramètres (APVTS)
  Presets/                                       gestion des presets
  Utils/                                         utilitaires transverses
  PluginProcessor.{h,cpp}                        processeur audio principal
  PluginEditor.{h,cpp}                           éditeur / UI racine
Resources/{Samples,Presets,Fonts,Icons}          ressources embarquées
Tests/{DSP,UI}                                    tests (CTest)
ThirdParty/                                        dépendances tierces éventuelles
docs/                                             documentation
```

## Licence

Projet personnel — licence à définir.

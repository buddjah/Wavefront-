# Découpage en phases — Wavefront

Développement **phase par phase**, avec validation avant de passer à la suivante.

## Phase 0 — Initialisation du dépôt et squelette ✅

**Objectif** : dépôt Git fonctionnel, projet JUCE/CMake qui compile en VST3
vide, chargeable dans un DAW de test.

Livré :
- dépôt Git initialisé, `.gitignore` adapté JUCE/CMake ;
- arborescence complète des dossiers (`Source/`, `Resources/`, `Tests/`, …) ;
- JUCE récupéré via `FetchContent` (tag épinglé dans `CMakeLists.txt`) ;
- `CMakeLists.txt` racine : projet « Wavefront », **VST3 uniquement**,
  `PluginProcessor`/`PluginEditor` minimalistes (plugin vide, pass-through) ;
- cible de tests CTest (`Tests/DSP/`) prête à recevoir les tests DSP.

## Phase 1 — Moteur Doppler/warping HQ

- `HQFractionalDelay` : interpolation de Lagrange 4 points ;
- oversampling ×4 via `juce::dsp::Oversampling` ;
- calcul distance → delay → pitch ;
- lissage anti-zipper ;
- filtre d'absorption atmosphérique.

## Phase 2 — Moteur granulaire + Matrice de modulation

- moteur granulaire synchronisé au playhead de la trajectoire, fenêtrage Hann,
  réutilisation de l'interpolateur HQ, normalisation par recouvrement ;
- matrice de modulation : 4 LFOs + sources dérivées de la trajectoire,
  connexions `{source, destination, profondeur, smoothing}`, calcul à taux
  audio pour les destinations critiques.

## Phase 3 — UI du Path Editor 2D

- spline éditable, playhead animé (communication lock-free thread audio → UI),
  modes Path / Listener / Measure.

## Phase 4 — Direction artistique néon / LookAndFeel

- `WavefrontLookAndFeel` custom ;
- mapping couleur par module : Path = violet, Physique = cyan, Sampler = vert,
  Effets = turquoise, Mod Matrix = orange ;
- glow via double-tracé.

## Phase 5 — Sampler complet, presets, polish, validation

- sampler complet, contenu factory (samples / presets) ;
- polish général ;
- validation `pluginval`.

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

## Phase 4 — Direction artistique néon ✅

- `WavefrontLookAndFeel` (rotatifs à arc néon avec glow par double tracé) ;
- palette d'accents par module (`WavefrontColours`) : Path=violet,
  Physique=cyan, Sampler=vert, Effets=turquoise, Mod Matrix=orange ;
- accent appliqué par module sur chaque contrôle de l'éditeur.

## Phase 5 — Effets post, presets, validation ✅ (sampler = suite)

Livré :
- chaîne d'effets post (`EffectsChain`) : EQ 3 bandes -> Distortion
  (oversamplée ×4 séparément) -> Compression -> Delay stéréo -> Tremolo,
  chaque étage avec bypass ; destinations de modulation câblées (trem depth,
  delay time, dist drive) ;
- gestion des presets (`PresetManager`) : 6 presets d'usine en code +
  presets utilisateur en XML (sauvegarde/chargement fichier), remise à zéro
  déterministe avant application ;
- UI : ComboBox de presets (Factory / User) + bouton Save (dialogue de nom) ;
- validation headless « torture test » du plugin (multi-fréquences,
  multi-blocs, entrées pathologiques, balayage de paramètres, round-trip
  d'état) — cible CTest `wavefront_plugin_validation`.

### Reste à faire (suite de la Phase 5)

- **Sampler de fichiers** : lecture d'échantillons audio chargés (glisser un
  WAV, le faire « voyager » sur la trajectoire) — distinct du moteur
  granulaire actuel qui granule le signal d'entrée. À spécifier/produire avec
  le contenu factory (samples), laissé volontairement en aval dans le cahier
  des charges.
- Contenu factory (banque de samples, presets additionnels).
- Passage `pluginval --strictness-level 10` sous Windows.

# Toolchain de cross-compilation Linux -> Windows (x86_64) via mingw-w64.
# Usage :
#   cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake ...
#
# Lie statiquement libgcc/libstdc++/winpthread pour que le VST3 ne dépende
# d'aucune DLL runtime sur la machine cible.
#
# LIMITATION : le rendu Windows de JUCE 8 est câblé sur Direct2D (code réservé
# à MSVC, sans repli) -> la cible GUI complète (VST3) NE compile PAS avec
# mingw-w64. Pour un vrai VST3 Windows, utiliser MSVC (voir le workflow
# .github/workflows/build-windows.yml qui build sur un runner windows-latest).
# Ce toolchain reste utile pour des builds Windows sans GUI (DSP/console) ou si
# l'on repasse un jour à un rendu logiciel/GDI.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# Variante "posix" : nécessaire pour std::thread / std::mutex utilisés par JUCE.
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc-posix)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++-posix)
set(CMAKE_RC_COMPILER  ${TOOLCHAIN_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# mingw masque les prototypes non standard (stricmp) sous -std=c++17 qui définit
# __STRICT_ANSI__ ; et certains en-têtes JUCE utilisent memset dans des templates
# sans inclure <cstring> (lookup en deux phases strict). On neutralise le premier
# et on pré-inclut <cstring> pour le C++.
set(CMAKE_C_FLAGS_INIT   "-U__STRICT_ANSI__")
set(CMAKE_CXX_FLAGS_INIT "-U__STRICT_ANSI__ -include cstring")

# Runtime statique -> VST3 autonome (pas de libstdc++-6.dll / libwinpthread-1.dll).
add_link_options(-static -static-libgcc -static-libstdc++)

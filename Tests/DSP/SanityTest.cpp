// Test "sanity" de la Phase 0.
//
// Objectif unique : prouver que la cible CTest compile, se lie et s'exécute.
// Aucun code DSP réel n'est encore testé ici — les tests du moteur
// Doppler/warping, granulaire et de la matrice de modulation viendront
// remplacer/compléter ce fichier dans les phases suivantes.
//
// Convention de retour : 0 = succès, non-zéro = échec (interprété par CTest).

#include <cmath>
#include <cstdio>

namespace
{
// Petit utilitaire d'assertion sans dépendance externe (framework de test à
// choisir plus tard : Catch2 / GoogleTest via FetchContent).
int failures = 0;

void check (bool condition, const char* what)
{
    if (! condition)
    {
        std::printf ("[FAIL] %s\n", what);
        ++failures;
    }
    else
    {
        std::printf ("[ OK ] %s\n", what);
    }
}
} // namespace

int main()
{
    // Vérifications triviales : arithmétique flottante de base.
    check (1 + 1 == 2, "arithmetique entiere");
    check (std::abs (0.1f + 0.2f - 0.3f) < 1.0e-6f, "arithmetique flottante");

    if (failures == 0)
        std::printf ("Sanity DSP : tous les tests passent.\n");

    return failures == 0 ? 0 : 1;
}

#include "integrate.hxx"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <functional>
#include <sstream>

/** Teste la convergence du calcul d'intégrale avec le nombre de rectangles.

    On utilise comme cas-test le calcul de l'intégrale de cos entre 0 et pi/2,
    dont la valeur exacte est 1.

    Le même calcul est réalisé pour une suite croissante (~géométrique) de
    nombre de rectangles. Chaque résultat de calcul est affiché en sortie.

    @param step  facteur entre deux nombre de rectangles testés
 */
void testConvergence (const RealType & step) {
  std::cout << std::scientific << std::setprecision(16);

  for (unsigned int n = 1 ;
       n <= 100000 ;
       n = std::max ((unsigned int)(step*n), n+1)) {
    // Calcul approché
    RealType res = integrate ((RealType(*)(RealType)) std::cos,
                              0, M_PI_2, n);

    // Erreur (relative) par rapport à la valeur exacte
    RealType err = std::abs(1 - res);

    // Affichage en sortie sur trois colonnes:
    //   Nrectangles   Resultat   Erreur
    std::cout << std::setw(10) << n << " " << res << " " << err << std::endl;
  }
}



/** Fonction utilitaire de conversion de chaîne

    @param str  la chaîne à convertir
    @param TO   le type de donnée à lire
 */
template <typename TO>
TO strTo (const std::string & str) {
  std::istringstream iss(str);
  TO x;
  iss >> x;
  return x;
}



/** Fonction principale : le facteur de croissance du nombre de rectangles
    testés est lu comme premier argument en ligne de commande. La valeur 10 est
    choisie par défaut si aucun argument n'est fourni.
 */
int main (int argc, char **argv) {
  RealType step = 10;
  if (argc > 1)
    step = strTo<RealType> (argv[1]);

  testConvergence (step);
  return 0;
}

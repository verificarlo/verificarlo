typedef float RealType;

/** Calcule l'intégrale d'une fonction entre deux bornes avec la méthode des
    rectangles.

    @param f   la fonction à intégrer
    @param x0  borne gauche de l'intervalle d'intégration
    @param x1  borne droite
    @param n   nombre de rectangles
 */
template <typename T>
RealType integrate (const T& f, RealType a, RealType b, unsigned int n) {
  // Pas d'intégration
  const RealType dx = (b - a) / n;

  // Accumulateur
  RealType sum = 0.;

  // Boucle sur les rectangles d'intégration
  for (unsigned int i = 0 ; i<n ; ++i) {
    RealType x = a + (i+0.5) * dx;
    RealType tmp = dx * f(x);
    sum += tmp;
  }

  return sum;
}

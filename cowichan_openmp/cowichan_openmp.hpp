/**
 * Datatypes and common routines for Cowichan programs.
 * OpenMP implementation.
 */
#ifndef __cowichan_openmp_hpp__
#define __cowichan_openmp_hpp__

#include "../cowichan/cowichan.hpp"

#include <omp.h>

// COWICHAN DEFINITIONS =====================================================//
// aka. "inputs" to the toys, and chaining functions.
class CowichanOpenMP : public Cowichan {
protected: // chaining functions

  void mandel(IntMatrix matrix);
  void randmat(IntMatrix matrix);
  void half(IntMatrix matrixIn, IntMatrix matrixOut);
  void invperc(IntMatrix matrix, BoolMatrix mask);
  void thresh(IntMatrix matrix, BoolMatrix mask);
  void life(BoolMatrix matrixIn, BoolMatrix matrixOut);
  void winnow(IntMatrix matrix, BoolMatrix mask, PointVector points);
  void norm(PointVector pointsIn, PointVector pointsOut);
  void hull(PointVector pointsIn, PointVector pointsOut);
  void outer(PointVector points, Matrix matrix, Vector vector);
  void gauss(Matrix matrix, Vector target, Vector solution);
  void sor(Matrix matrix, Vector target, Vector solution);
  void product(Matrix matrix, Vector candidate, Vector solution);
  real vecdiff(Vector actual, Vector computed);

protected:

  /**
   * Runs the cowichan problem set, chained together.
   * @param use_randmat  true: generate a random matrix.
   *             false: use a window of the mandelbrot set.
   * @param use_thresh  true: use image thresholding for int->bool.
   *            false: use invasion percolation for int->bool.
   */
  void chain(bool use_randmat, bool use_thresh);

public:

  /**
   * Cutoff n value for hull
   */
  static const int HULL_CUTOFF = 20000;

};

#endif

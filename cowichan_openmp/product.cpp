#include "cowichan_openmp.hpp"

void CowichanOpenMP::product (Matrix matrix, Vector candidate, Vector solution)
{
  INT64 r, c;

#pragma omp parallel for schedule(static) private(c)
  for (r = 0; r < n; r++) {
    solution[r] = MATRIX_SQUARE(matrix, r, 0) * candidate[0];
    for (c = 1; c < n; c++) {
      solution[r] += MATRIX_SQUARE(matrix, r, c) * candidate[c];
    }
  }
}

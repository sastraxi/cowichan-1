#include "cowichan_serial.hpp"

INT64 mask_count(BoolMatrix mask, INT64 nr, INT64 nc);

void not_enough_points();

void CowichanSerial::winnow(IntMatrix matrix, BoolMatrix mask,
    PointVector points) {

  INT64 r, c;
  INT64 len; // number of points
  INT64 stride; // selection stride
  INT64 i, j;

  // count set cell
  len = mask_count (mask, nr, nc);

  if (len < n) {
    not_enough_points();
  }

  WeightedPointVector weightedPoints = NULL;
  try {
    weightedPoints = NEW_VECTOR_SZ(WeightedPoint, len);
  }
  catch (...) {out_of_memory();}

  // fill temporary vector
  i = 0;
  for (r = 0; r < nr; r++) {
    for (c = 0; c < nc; c++) {
      if (MATRIX_RECT(mask, r, c)) {
        weightedPoints[i++] = WeightedPoint((real)c, (real)r,
            MATRIX_RECT(matrix, r, c));
      }
    }
  }

#ifdef SORT_TIME
  INT64 start, end;
  start = get_ticks ();
#endif

  // sort
  std::sort(weightedPoints, &weightedPoints[len]);
  
#ifdef SORT_TIME
  end = get_ticks ();
#endif

  // copy over points
  stride = len / n;

  for (i = n - 1, j = len - 1; i >= 0; i--, j -= stride) {
#ifdef WINNOW_OUTPUT
    std::cout << weightedPoints[j].weight << "\n";
#endif
    points[i] = weightedPoints[j].point;
  }
  
#ifdef SORT_TIME
  std::cout << "winnow sort: ";
  print_elapsed_time(start, end);
  std::cout << std::endl;
#endif

  delete [] weightedPoints;

}

/**
 * Count the number of set cells in the mask.
 * @param mask boolean mask.
 * @param nr number of rows.
 * @param nc number of columns.
 */
INT64 mask_count(BoolMatrix mask, INT64 nr, INT64 nc) {

  INT64 r, c, sum = 0;

  for (r = 0; r < nr; r++) {
    for (c = 0; c < nc; c++) {
      if (MATRIX_RECT_NC(mask, r, c, nc)) {
        sum++;
      }
    }
  }

  return sum;
}



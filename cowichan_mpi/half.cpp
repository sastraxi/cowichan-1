/**
 * \file cowichan_mpi/half.cpp
 * \brief MPI halving shuffle implementation.
 * \see CowichanMPI::half
 */

#include "cowichan_mpi.hpp"
void CowichanMPI::half (IntMatrix matrixIn, IntMatrix matrixOut)
{
  bool work;
  int		lo, hi;		/* work controls */
  int		rlo, rhi;		/* work controls */
  int		r, c, i;		/* loop indices */

  int middle_r = (nr + 1) / 2;
  int middle_c = (nc + 1) / 2;
  int previous_r, previous_c;

  IntMatrix tmp_matrix = NEW_MATRIX_RECT(INT_TYPE);

  // work
  work = get_block(world, 0, nr, &lo, &hi);

  // shuffle along rows
  if (work) {
    for (r = lo; r < hi; r++) {
      for (c = 0; c < nc; c++) {
        		// get original column
				if (c < middle_c) {
					previous_c = c * 2;
				} else {
					previous_c = (c - middle_c) * 2 + 1;
        }
        MATRIX_RECT(tmp_matrix, r, c) = MATRIX_RECT(matrixIn, r, previous_c);
      }
    }
  }
  
  // broadcast tmp_matrix rows
  for (i = 0; i < world.size (); i++) {
    if (get_block(world, 0, nr, &rlo, &rhi, i)) {
      broadcast (world, &MATRIX_RECT(tmp_matrix, rlo, 0), (rhi - rlo) * nc, i);
    }
  }

  // shuffle along columns
  if (work) {
    for (r = lo; r < hi; r++) {
      for (c = 0; c < nc; c++) {
        		// get original row
				if (r < middle_r) {
					previous_r = r * 2;
				} else {
					previous_r = (r - middle_r) * 2 + 1;
        }
        MATRIX_RECT(matrixOut, r, c) = MATRIX_RECT(tmp_matrix, previous_r, c);
      }
    }
  }

  // broadcast matrix rows
  for (i = 0; i < world.size (); i++) {
    if (get_block(world, 0, nr, &rlo, &rhi, i)) {
      broadcast (world, &MATRIX_RECT(matrixOut, rlo, 0), (rhi - rlo) * nc, i);
    }
  }

  // cleanup
  delete [] tmp_matrix;

  /* return */
}


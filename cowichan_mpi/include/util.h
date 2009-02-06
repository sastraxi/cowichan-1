/**
 * Utility functions
 *
 * \file util.h
 * \author Andrew Borzenko
 * \date 01-26-09
 */

void
fail(
  char	      * caller,			/* calling function */
  char	      * descrip,		/* error description */
  ...					/* other things to print */
);

void print_matrix (bool2D matrix, int nr, int nc);
void print_matrix (int2D matrix, int nr, int nc);
void print_matrix (real2D matrix, int nr, int nc);
void print_matrix (bool1DX matrix, int nr, int nc);
void print_matrix (int1DX matrix, int nr, int nc);
void print_matrix (real1DX matrix, int nr, int nc);
void print_vector (bool1D vector, int nr);
void print_vector (int1D vector, int nr);
void print_vector (real1D vector, int nr);

bool get_block_rows_mpi (mpi::communicator world, int lo, int hi,
                         int* start, int* end, int* stride);
int get_block_rank_mpi (mpi::communicator world, int lo, int hi,
                        int row);
bool get_cyclic_rows_mpi (mpi::communicator world, int lo, int hi,
                         int* start, int* end, int* stride);
int get_cyclic_rank_mpi (mpi::communicator world, int lo, int hi,
                        int element);

void randStateInit (unsigned int seed,       /* RNG seed */
                    int width,      /* number of participants */
                    unsigned int	* state,    /* per-thread state vector */
                    unsigned int	* aPrime,   /* new multiplicative */
                    unsigned int	* cPrime);  /* new additive value */
/*
    Copyright (C) 2011 Serge Monkewitz

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License v3 as published
    by the Free Software Foundation, or any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    A copy of the LGPLv3 is available at <http://www.gnu.org/licenses/>.

    Authors:
        - Serge Monkewitz, IPAC/Caltech

    Work on this project has been sponsored by LSST and SLAC/DOE.

    ----------------------------------------------------------------

    Functions for selecting values from an array of doubles.
  */
#ifndef SCISQL_SELECT_H
#define SCISQL_SELECT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ---- Selection functions ---- */

/*  Finds the k-th smallest value in an array of doubles (where k = 0 is the
    smallest element) using the linear time median-of-medians algorithm.
    Except for pathological inputs (e.g. arrays of mostly/entirely identical
    values and other median-of-3 killer sequences), scisql_select() will be
    faster and should be preferred. Note that scisql_select() detects
    such pathological sequences and switches to the median-of-medians
    algorithm if necessary, so its runtime is also linear.

    This function has the same inputs ond enforces the same invariants as
    scisql_select().
 */
SCISQL_LOCAL double scisql_selectmm(double *array, size_t n, size_t k);

/*  Returns the k-th smallest value in an array of doubles (where k = 0 is
    the smallest element). The implementation guarantees O(n) runtime
    even when faced with an array of identical elements. After this function
    returns, the k-th largest element is stored in array[k], and the
    following invariants hold:

    -   array[i] <= array[k] for i < k
    -   array[i] >= array[k] for i > k

    If array == 0, n == 0 or k >= n, then a quiet NaN is returned.

    Inputs:
        array    Array to select from.
        n        Number of elements in array.
        k        The element to select.
 */
SCISQL_LOCAL double scisql_select(double *array, size_t n, size_t k);

/*  Returns the smallest value in an array of doubles.

    If array == 0 or n == 0, then a quiet NaN is returned.

    Inputs:
        array    Array to select from.
        n        Number of elements in array.
 */
SCISQL_LOCAL double scisql_min(const double *array, size_t n);


/* ---- Median/percentile ---- */

#define SCISQL_MMAP_FSIZE   (((size_t) 1) << 31)
#define SCISQL_MALLOC_SLOTS 8192
#define SCISQL_MAX_NELEMS   (SCISQL_MMAP_FSIZE / sizeof(double))


/*  A structure that tracks a set of input values from which a
    median/percentile can be computed.

    The implementation uses malloc for the first SCISQL_MALLOC_SLOTS
    values.  If more are added, a memory mapped file in /tmp is used
    instead.

    At the moment, a maximum of 2^28, or ~270 million values can be
    handled (SCISQL_MAX_NELEMS).
*/
typedef struct {
    size_t n;           /* number of values stored */
    double fraction;    /* percentage divided by 100 */
    double *malloc_buf; /* value buffer allocated with malloc */
    double *mmap_buf;   /* value buffer allocated with mmap and backed by fd */
    int fd;             /* descriptor for file backing mmap_buf */
} scisql_percentile_state;


/*  Creates and initializes a new scisql_percentile_state structure.
 */
SCISQL_LOCAL scisql_percentile_state * scisql_percentile_state_new();

/*  Frees any resources allocated by scisql_percentile_new() and
    scisql_percentile_add().
 */
SCISQL_LOCAL void scisql_percentile_state_free(scisql_percentile_state *p);

/*  Resets the state of a scisql_percentile structure without freeing
    any resources.
 */
SCISQL_LOCAL void scisql_percentile_state_clear(scisql_percentile_state *p);

/*  Adds a value to a scisql_percentile_state structure.
 */
SCISQL_LOCAL int scisql_percentile_state_add(scisql_percentile_state *p,
                                             double *value);

/*  Computes the percentile of the values tracked by p.
 */
SCISQL_LOCAL double scisql_percentile(scisql_percentile_state *p);


#ifdef __cplusplus
}
#endif

#endif /* SCISQL_SELECT_H */

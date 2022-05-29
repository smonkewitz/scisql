/*
    Copyright (C) 2011-2022 the SciSQL authors

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    Authors:
        - Serge Monkewitz, IPAC/Caltech

    Work on this project has been sponsored by LSST and SLAC/DOE.

    ----------------------------------------------------------------

    This file contains the implementation of functions declared in "select.h".
*/

#include "select.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif


/* ---- Implementation details ---- */

/*  Lookup tables for the 4 and 5 element median finding algorithms.
    Computed with the following python 2.6+ script (with n = 4, 5):

    @code
    import itertools

    def computeLut(n):
        nbits = (n * (n - 1)) / 2
        lut = [-1] * 2**nbits
        array = range(n)
        median = array[len(array) >> 1]
        for p in itertools.permutations(array):
            res = []
            for i in xrange(n - 1):
                for j in xrange(i + 1, n):
                    res.append(1 if p[i] < p[j] else 0)
            index = 0
            for i in xrange(len(res)):
                index += res[i] << (len(res) - i - 1)
            lut[index] = p.index(median)
        return lut
    @endcode
 */
static const signed char _lut4[64] = {
      1, 1,-1, 3, 2,-1, 2, 3,-1,-1,-1, 0,-1,-1,-1, 0,
     -1,-1,-1,-1, 0,-1, 0,-1,-1,-1,-1,-1,-1,-1, 3, 2,
      0, 0,-1,-1,-1,-1,-1,-1,-1, 3,-1, 1,-1,-1,-1,-1,
      2,-1,-1,-1, 1,-1,-1,-1, 2, 3,-1, 1, 1,-1, 3, 2
};
static const signed char _lut5[1024] = {
     2, 2,-1, 4, 3,-1, 3, 4,-1,-1,-1, 1,-1,-1,-1, 1,-1,-1,-1,-1, 1,-1, 1,-1,-1,-1,-1,-1,-1,-1, 4, 3,
     1, 1,-1,-1,-1,-1,-1,-1,-1, 4,-1, 2,-1,-1,-1,-1, 3,-1,-1,-1, 2,-1,-1,-1, 3, 4,-1, 2, 2,-1, 4, 3,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 3,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1, 3,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1, 1,-1,-1,-1,-1,-1,-1,-1, 4,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1,-1, 2,-1, 4,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     1, 1,-1,-1,-1,-1,-1,-1,-1, 4,-1,-1,-1,-1,-1,-1, 3,-1,-1,-1,-1,-1,-1,-1, 3, 4,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1, 0,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1,-1, 0,-1,-1,-1, 0,-1,-1,-1, 0,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 4, 3,-1, 3, 4,-1, 2, 2,
     2, 2,-1, 4, 3,-1, 3, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1, 0,-1,-1,-1, 0,-1,-1,-1, 0,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1, 0,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1, 4, 3,-1,-1,-1,-1,-1,-1,-1, 3,-1,-1,-1,-1,-1,-1, 4,-1,-1,-1,-1,-1,-1,-1, 1, 1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     0, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     0, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1, 4,-1, 2,-1,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1, 4,-1,-1,-1,-1,-1,-1,-1, 1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     3,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     3, 4,-1, 2, 2,-1, 4, 3,-1,-1,-1, 2,-1,-1,-1, 3,-1,-1,-1,-1, 2,-1, 4,-1,-1,-1,-1,-1,-1,-1, 1, 1,
     3, 4,-1,-1,-1,-1,-1,-1,-1, 1,-1, 1,-1,-1,-1,-1, 1,-1,-1,-1, 1,-1,-1,-1, 4, 3,-1, 3, 4,-1, 2, 2
};


/*  Returns the index of the median of 2 doubles.
 */
static __inline size_t _median2(const double *array) {
    return (array[0] < array[1]) ? 0 : 1;
}

/*  Returns the index of the median of 3 doubles.
 */
static __inline size_t _median3(const double *array) {
    double v0 = array[0];
    double v1 = array[1];
    double v2 = array[2];
    if (v0 < v1) {
        return v1 < v2 ? 1 : (v0 < v2 ? 2 : 0);
    } else {
        return v1 < v2 ? (v0 < v2 ? 0 : 2) : 1;
    }
}

/*  Returns the index of the median of 4 doubles using a branchless algorithm.

    TODO: investigate whether using branches and a smaller number of
    comparisons is faster.
 */
static __inline size_t _median4(const double *array) {
    double a = array[0];
    double b = array[1];
    double c = array[2];
    double d = array[3];
    /* avoids branches, but always requires 6 comparisons. */
    int i = (((int) (a < b)) << 5) |
            (((int) (a < c)) << 4) |
            (((int) (a < d)) << 3) |
            (((int) (b < c)) << 2) |
            (((int) (b < d)) << 1) |
             ((int) (c < d));
    return (size_t) _lut4[i];
}

/*  Returns the index of the median of 5 doubles using a branchless algorithm.

    TODO: investigate whether using branches and a smaller number of
    comparisons is faster.
 */
static __inline size_t _median5(const double *array) {
    double a = array[0];
    double b = array[1];
    double c = array[2];
    double d = array[3];
    double e = array[4];
    /* avoids branches, but always performs 10 comparisons */
    int i = (((int) (a < b)) << 9) |
            (((int) (a < c)) << 8) |
            (((int) (a < d)) << 7) |
            (((int) (a < e)) << 6) |
            (((int) (b < c)) << 5) |
            (((int) (b < d)) << 4) |
            (((int) (b < e)) << 3) |
            (((int) (c < d)) << 2) |
            (((int) (c < e)) << 1) |
             ((int) (d < e));
    return (size_t) _lut5[i];
}

/*  Returns the index of the median of medians for an array.

    The following pre-conditions are assumed:
        -   array != 0
        -   n > 0
 */
static size_t _medianOfMedians(double *array, size_t n) {
    size_t i, j, m=0;
    while (1) {
        if (n <= 5) {
            switch (n) {
                case 1: m = 0; break;
                case 2: m = _median2(array); break;
                case 3: m = _median3(array); break;
                case 4: m = _median4(array); break;
                case 5: m = _median5(array); break;
            }
            break;
        }
        for (i = 0, j = 0; i < n - 4; i += 5, ++j) {
            size_t m5 = _median5(array + i) + i;
            double tmp = array[j];
            array[j] = array[m5];
            array[m5] = tmp;
        }
        n = j;
    }
    return m;
}

/*  Partitions the given array around the value of the i-th element, and
    returns the index of the pivot value after partitioning.

    If the array contains many values identical to the pivot value,
    lop-sided partitions can be generated by a naive algorithm. In the
    worst case (e.g. all elements are identical), an array of n elements will
    be partitioned into two sub-arrays of size 1 and n - 1. This leads to
    quadratic run-time for selection/sorting algorithms that use the
    partitioning primitive.

    This implementation therefore counts values identical to the pivot
    while partitioning. If the partitions are overly lop-sided, a second
    pass over the larger partition is performed. This pass assigns pivot
    values to the smaller partition until the sizes are balanced or there
    are no more pivot values left.

    The run-time of this function is O(n).

    The following pre-conditions are assumed:
        -   array != 0
        -   n > 0
        -   i < n
 */
static size_t _worstCasePartition(double *array, size_t n, size_t i) {
    size_t u, v, neq;
    const double pivot = array[i];
    array[i] = array[n - 1];
    /* partition around pivot */
    for (u = 0, v = 0, neq = 0; v < n - 1; ++v) {
        if (array[v] < pivot) {
            double tmp = array[u];
            array[u] = array[v];
            array[v] = tmp;
            ++u;
        } else if (array[v] == pivot) {
            ++neq;
        }
    }
    array[n - 1] = array[u];
    array[u] = pivot;
    if (neq > 0 && u < (n >> 2)) {
        /* lop-sided partition - use values identical
           to the pivot value to increase u */
        if (u + neq > (n >> 1)) {
            neq = (n >> 1) - u;
        }
        for (v = u + 1; neq != 0; ++v) {
            if (array[v] == pivot) {
                ++u;
                array[v] = array[u];
                array[u] = pivot;
                --neq;
            }
        }
    }
    return u;
}

/*  Chooses a pivot value using the median-of-3 strategy.

    The following pre-conditions are assumed:
        -   array != 0
        -   n > 0
 */
static size_t _median3Pivot(double *array, size_t n) {
    double a, b, c;
    size_t m;

    if (n <= 5) {
        switch (n) {
            case 1: return 0;
            case 2: return _median2(array);
            case 3: return _median3(array);
            case 4: return _median4(array);
            case 5: return _median5(array);
        }
    }
    m = n >> 1;
    a = array[0];
    b = array[m];
    c = array[n - 1];
    if (a < b) {
        return b < c ? m : (a < c ? n - 1 : 0);
    } else {
        return b < c ? (a < c ? 0 : n - 1) : m;
    }
}

/*  Partitions the given array around the value of the i-th element, and
    returns the index of the pivot value after partitioning.

    The following pre-conditions are assumed:
        -   array != 0
        -   n > 0
        -   i < n
 */
static size_t _partition(double *array, size_t n, size_t i) {
    size_t u, v;
    const double pivot = array[i];
    array[i] = array[n - 1];
    for (u = 0, v = 0; v < n - 1; ++v) {
        if (array[v] < pivot) {
            double tmp = array[u];
            array[u] = array[v];
            array[v] = tmp;
            ++u;
        }
    }
    array[n - 1] = array[u];
    array[u] = pivot;
    return u;
}


/* ---- Selection functions ---- */

static const double SCISQL_QNAN = 0.0 / 0.0;


SCISQL_LOCAL double scisql_selectmm(double *array, size_t n, size_t k) {
    if (array == 0 || n == 0 || k >= n) {
        return SCISQL_QNAN;
    }
    while (1) {
        size_t i = _medianOfMedians(array, n);
        i = _worstCasePartition(array, n, i);
        if (k == i) {
            break;
        } else if (k < i) {
            n = i;
        } else {
            array += i + 1;
            n -= i + 1;
            k -= i + 1;
        }
    }
    return array[k];
}


/*  This implementation uses the quickselect algorithm with median-of-3
    pivots.  The quadratic worst case is detected by keeping a running sum
    of the partition sizes generated so far.  When this sum exceeds 3*n,
    we switch to the worst-case linear median-of-medians algorithm.
 */
SCISQL_LOCAL double scisql_select(double *array, size_t n, size_t k) {
    const size_t thresh = n*3;
    size_t tot = 0;

    if (array == 0 || n == 0 || k >= n) {
        return SCISQL_QNAN;
    }
    while (1) {
        size_t i = _median3Pivot(array, n);
        i = _partition(array, n, i);
        if (k == i) {
            break;
        } else if (k < i) {
            n = i;
        } else {
            array += i + 1;
            n -= i + 1;
            k -= i + 1;
        }
        tot += n;
        if (tot > thresh) {
            return scisql_selectmm(array, n, k);
        }
    }
    return array[k];
}


SCISQL_LOCAL double scisql_min(const double *array, size_t n) {
    double m;
    size_t i;

    if (array == 0 || n == 0) {
        return SCISQL_QNAN;
    }
    m = array[0];
    for (i = 1; i < n; ++i) {
        if (array[i] < m) {
            m = array[i];
        }
    }
    return m;
}


/* ---- Median/percentile ---- */

SCISQL_LOCAL scisql_percentile_state * scisql_percentile_state_new() {
    scisql_percentile_state *p =
        (scisql_percentile_state *) malloc(sizeof(scisql_percentile_state));
    if (p != 0) {
        p->n = 0;
        p->fraction = 0.5;
        p->fd = -1;
        p->mmap_buf = 0;
        p->malloc_buf = (double *) malloc(SCISQL_MALLOC_SLOTS * sizeof(double));
        if (p->malloc_buf == 0) {
            free(p);
            p = 0;
        }
    }
    return p;
}


SCISQL_LOCAL void scisql_percentile_state_free(scisql_percentile_state *p) {
    if (p != 0) {
        if (p->fd != -1) {
            munmap(p->mmap_buf, SCISQL_MMAP_FSIZE);
            close(p->fd);
            p->mmap_buf = 0;
            p->fd = -1;
        }
        free(p->malloc_buf);
        p->malloc_buf = 0;
        p->n = 0;
        free(p);
    }
}


SCISQL_LOCAL void scisql_percentile_state_clear(scisql_percentile_state *p) {
    if (p != 0) {
        p->n = 0;
    }
}


SCISQL_LOCAL int scisql_percentile_state_add(scisql_percentile_state *p,
                                             double *value)
{
    double v;
    size_t n;

    if (p == 0) {
        return 1;
    }
    if (value == 0) {
        return 0;
    }
    v = *value;
    if (SCISQL_ISNAN(v)) {
        return 0;
    }
    n = p->n;
    if (n < SCISQL_MALLOC_SLOTS) {
        p->malloc_buf[n] = v;
    } else {
        if (n == SCISQL_MAX_NELEMS) {
            return 1;
        } else if (n == SCISQL_MALLOC_SLOTS) {
            if (p->fd == -1) {
                char fname[32];
                double *buf;
                int fd, prot, flgs;

                strcpy(fname, "/tmp/scisql_select_XXXXXX");
                /* create temp file */
                fd = mkstemp(fname);
                if (fd == -1) {
                    fprintf(stderr, "scisql_percentile_state_add: mkstemp "
                            "failed for %s, errno: %i\n", fname, errno); 
                    return 1;
                }
                /* guard against other processes using the file */
                if (fchmod(fd, S_IRUSR | S_IWUSR) != 0) {
                    unlink(fname);
                    close(fd);
                    fprintf(stderr, "scisql_percentile_state_add: chmod "
                            "failed for %s, errno: %i\n", fname, errno); 
                    return 1;
                }
                /* unlink it immediately */
                if (unlink(fname) != 0) {
                    close(fd);
                    fprintf(stderr, "scisql_percentile_state_add: unlink "
                            "failed for %s, errno: %i\n", fname, errno); 
                    return 1;
                }
                /* adjust file size */
                if (ftruncate(fd, SCISQL_MMAP_FSIZE) != 0) {
                    close(fd);
                    fprintf(stderr, "scisql_percentile_state_add: ftruncate "
                            "failed for %s, errno: %d\n", fname, errno); 
                    return 1;
                }
                /* memory map it */
                prot = PROT_READ | PROT_WRITE;
                flgs = MAP_SHARED;
#if MAP_HUGETLB
                flgs |= MAP_HUGETLB;
#endif
                buf = (double *) mmap(0, SCISQL_MMAP_FSIZE, prot, flgs, fd, 0);
                if (buf == MAP_FAILED) {
#if MAP_HUGETLB
                    /* try again without huge pages */
                    flgs = MAP_SHARED;
                    buf = (double *) mmap(0, SCISQL_MMAP_FSIZE, prot, flgs, fd, 0);
                    if (buf == MAP_FAILED) {
#endif
                        close(fd);
                        fprintf(stderr, "mmap failed for %s\n", fname); 
                        return 1;
#if MAP_HUGETLB
                    }
#endif
                }
                p->mmap_buf = buf;
                p->fd = fd;
            }
            /* copy data to memory mapped file and clean up */
            memcpy(p->mmap_buf, p->malloc_buf, n * sizeof(double));
        }
        p->mmap_buf[n] = v;
    }
    p->n = n + 1;
    return 0;
}


SCISQL_LOCAL double scisql_percentile_state_get(scisql_percentile_state *p) {
    size_t n, k;
    double val, frac, i, rem;
    double *array;

    if (p == 0) {
        return SCISQL_QNAN;
    }
    n = p->n;
    if (n == 0) {
        return SCISQL_QNAN;
    }
    frac = p->fraction;
    if (SCISQL_ISNAN(frac) || frac < 0.0 || frac > 1.0) {
        return SCISQL_QNAN;
    }
    if (n == 1) {
        return p->malloc_buf[0];
    }
    array = (n <= SCISQL_MALLOC_SLOTS) ? p->malloc_buf : p->mmap_buf;
    i = frac * (n - 1);
    k = (size_t) floor(i);
    rem = i - k;
    val = scisql_select(array, n, k);
    if (rem != 0.0) {
        // k is at most n - 2
        val += rem * (scisql_min(array + (k + 1), n - (k + 1)) - val);
    }
    return val;
}


#ifdef __cplusplus
}
#endif


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

    A minimalistic set of functions and types for HTM indexing.

    This software is based on work by A. Szalay, T. Budavari,
    G. Fekete at The Johns Hopkins University, and Jim Gray,
    Microsoft Research. See the following links for more information:

    http://voservices.net/spherical/
    http://adsabs.harvard.edu/abs/2010PASP..122.1375B
 */
#ifndef SCISQL_HTM_H
#define SCISQL_HTM_H

#include <stdint.h>

#include "common.h"
#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Maximum HTM tree subdivision level */
#define SCISQL_HTM_MAX_LEVEL 24

/*  A sorted list of 64 bit integer ranges.
 */
typedef struct {
    size_t n;         /* number of ranges in list */
    size_t cap;       /* capacity of the range list */
    int64_t ranges[]; /* ranges [min_i, max_i], with min_i <= max_i
                         and min_j > max_i for all j > i. */
} scisql_ids;

/*  A 3-vector and a pointer to an associated payload.
 */
typedef struct {
    scisql_v3 v;
    void * payload;
} scisql_v3p SCISQL_ALIGNED(16);

/*  Computes an HTM ID for a position.

    Returns -1 if v is 0 or level is not in [0, SCISQL_HTM_MAX_LEVEL].
    Valid IDs are always positive.
 */
SCISQL_LOCAL int64_t scisql_v3_htmid(const scisql_v3 *point, int level);

/*  Computes HTM IDs for a list of positions with payloads. Positions
    and payloads are sorted by HTM ID during the id generation process.

    Returns 0 on success and 1 on error.
 */
SCISQL_LOCAL int scisql_v3_htmsort(scisql_v3p *points,
                                   int64_t *ids,
                                   size_t n,
                                   int level);

/*  Computes a list of HTM ID ranges corresponding to the HTM triangles
    overlapping the given circle.

    Inputs:
        ids     Existing id range list or 0. If this argument is null, a
                fresh range list is allocated and returned. If it is non-null,
                all its entries are removed, but its memory is re-used. This
                can be used to avoid malloc/realloc costs when this function
                is being called inside of a loop.
        center  Center of circle.
        radius  Circle radius, degrees.
        level   Subdivision level, [0, SCISQL_HTM_MAX_LEVEL].

    Return:
        A list of HTM ID ranges for the HTM triangles overlapping the given
        circle. A null pointer is returned if center == 0, radius is negative
        or level is not in the range [0, SCISQL_HTM_MAX_LEVEL], or if an
        internal memory (re)allocation fails.

        Note that the input range list may be reallocated (to grow its
        capacity), and so the input range list pointer may no longer point to
        valid memory. Always replace the input pointer with the return value
        of this function!

        If a memory (re)allocation fails, this function will free the memory
        associated with the range list (even it came from a non-null input
        pointer).

        A pointer to a scisql_ids can be cleaned up simply by passing it to
        free().
 */
SCISQL_LOCAL scisql_ids * scisql_s2circle_htmids(scisql_ids *ids,
                                                 const scisql_v3 *center,
                                                 double radius,
                                                 int level);

/*  Computes a list of HTM ID ranges corresponding to the HTM triangles
    overlapping the given spherical convex polygon.

    Inputs:
        ids     Existing id range list or 0. If this argument is null, a
                fresh range list is allocated and returned. If it is non-null,
                all its entries are removed, but its memory is re-used. This
                can be used to avoid malloc/realloc costs when this function
                is being called inside of a loop.
        poly    Spherical convex polygon.
        level   Subdivision level, [0, SCISQL_HTM_MAX_LEVEL].

    Return:
        A list of HTM ID ranges for the HTM triangles overlapping the given
        polygon. A null pointer is returned if poly == 0 or level is not in
        the range [0, SCISQL_HTM_MAX_LEVEL], or if an internal memory
        (re)allocation fails.

        Note that the input range list may be reallocated (to grow its
        capacity), and so the input range list pointer may no longer point to
        valid memory. Always replace the input pointer with the return value
        of this function!

        If a memory (re)allocation fails, this function will free the memory
        associated with the range list (even it came from a non-null input
        pointer).

        A pointer to a scisql_ids can be cleaned up simply by passing it to
        free().
 */
SCISQL_LOCAL scisql_ids * scisql_s2cpoly_htmids(scisql_ids *ids,
                                                const scisql_s2cpoly *poly,
                                                int level);

#ifdef __cplusplus
}
#endif

#endif /* SCISQL_HTM_H */


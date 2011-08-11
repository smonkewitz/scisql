/*
    Copyright (C) 2011 Serge Monkewitz

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

/* Maximum size of a BLOB representation of an HTM ID range list */
#define SCISQL_HTM_MAX_BLOB_SIZE (16*1024*1024)

/* Maximum number of ranges in a BLOB representation of an HTM ID range list */
#define SCISQL_HTM_MAX_RANGES (SCISQL_HTM_MAX_BLOB_SIZE / (2*sizeof(int64_t)))

/*  Root triangle numbers. The HTM ID of a root triangle is its number plus 8.
 */
typedef enum {
    SCISQL_HTM_S0 = 0,
    SCISQL_HTM_S1 = 1,
    SCISQL_HTM_S2 = 2,
    SCISQL_HTM_S3 = 3,
    SCISQL_HTM_N0 = 4,
    SCISQL_HTM_N1 = 5,
    SCISQL_HTM_N2 = 6,
    SCISQL_HTM_N3 = 7,
    SCISQL_HTM_NROOTS = 8
} scisql_htmroot;

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
SCISQL_LOCAL int scisql_v3p_htmsort(scisql_v3p *points,
                                    int64_t *ids,
                                    size_t n,
                                    int level);

/*  Computes a list of HTM ID ranges corresponding to the HTM triangles
    overlapping the given circle.

    Inputs:
        ids        Existing id range list or 0. If this argument is null,
                   a fresh range list is allocated and returned. If it is
                   non-null, all its entries are removed, but its memory is
                   re-used. This can be used to avoid malloc/realloc costs
                   when this function is being called inside of a loop.
        center     Center of circle, must be a unit vector.
        radius     Circle radius, degrees.
        level      Subdivision level, [0, SCISQL_HTM_MAX_LEVEL].
        maxranges  Maximum number of ranges to return. When too many ranges
                   are generated, the effective subdivision level of HTM ids
                   is reduced. Since two consecutive ranges that cannot be
                   merged at level L may become mergable at level L-n, this
                   "coarsening" cuts down on the number of ranges (but makes
                   the range list a poorer approximation to the input
                   geometry). Note that for arbitrary input geometry, up to
                   4 ranges may be generated no matter what the subdivision
                   level is. So for maxranges < 4, the requested bound may
                   not be achieved.

    Return:
        A list of HTM ID ranges for the HTM triangles overlapping the given
        circle. A null pointer is returned if center == 0 or level is not in
        the range [0, SCISQL_HTM_MAX_LEVEL], or if an internal memory
        (re)allocation fails.

        Note that the input range list may be reallocated (to grow its
        capacity), and so the input range list pointer may no longer point to
        valid memory. Always replace the input pointer with the return value
        of this function!

        If a memory (re)allocation fails, this function will free the memory
        associated with the range list (even it came from a non-null input
        pointer).

        A pointer to a scisql_ids struct can be cleaned up simply by passing
        it to free().
 */
SCISQL_LOCAL scisql_ids * scisql_s2circle_htmids(scisql_ids *ids,
                                                 const scisql_v3 *center,
                                                 double radius,
                                                 int level,
                                                 size_t maxranges);

/*  Computes a list of HTM ID ranges corresponding to the HTM triangles
    overlapping the given spherical convex polygon.

    Inputs:
        ids        Existing id range list or 0. If this argument is null,
                   a fresh range list is allocated and returned. If it is
                   non-null, all its entries are removed, but its memory is
                   re-used. This can be used to avoid malloc/realloc costs
                   when this function is being called inside of a loop.
        poly       Spherical convex polygon.
        level      Subdivision level, [0, SCISQL_HTM_MAX_LEVEL].
        maxranges  Maximum number of ranges to return. When too many ranges
                   are generated, the effective subdivision level of HTM ids
                   is reduced. Since two consecutive ranges that cannot be
                   merged at level L may become mergable at level L-n, this
                   "coarsening" cuts down on the number of ranges (but makes
                   the range list a poorer approximation to the input
                   geometry). Note that for arbitrary input geometry, up to
                   4 ranges may be generated no matter what the subdivision
                   level is. So for maxranges < 4, the requested bound may
                   not be achieved.

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

        A pointer to a scisql_ids struct can be cleaned up simply by passing
        it to free().
 */
SCISQL_LOCAL scisql_ids * scisql_s2cpoly_htmids(scisql_ids *ids,
                                                const scisql_s2cpoly *poly,
                                                int level,
                                                size_t maxranges);

#ifdef __cplusplus
}
#endif

#endif /* SCISQL_HTM_H */


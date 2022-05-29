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
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "htm.h"


#define SCISQL_ASSERT(pred, ...) \
    do { \
        if (!(pred)) { \
            fprintf(stderr, #pred " is false: " __VA_ARGS__); \
            fprintf(stderr, "\n"); \
            exit(1); \
        } \
    } while(0)

#define SQRT_2_2 0.707106781186547524400844362105 /* sqrt(2)/2 */
#define SQRT_3_3 0.577350269189625764509148780503 /* sqrt(3)/3 */
#define C0       0.270598050073098492199861602684 /* 1 / (2 * sqrt(2 + sqrt(2))) */
#define C1       0.923879532511286756128183189400 /* (1 + sqrt(2)) /
                                                     (sqrt(2) * sqrt(2 + sqrt(2))) */

#define NTEST_POINTS 50 /* number of entries in test_points */
#define CENTERS      18 /* Index of first HTM triangle center in test_points */

static const scisql_v3p test_points[NTEST_POINTS] = {
    { {       1.0,       0.0,       0.0 }, 0 }, /*  x */
    { {       0.0,       1.0,       0.0 }, 0 }, /*  y */
    { {       0.0,       0.0,       1.0 }, 0 }, /*  z */
    { {      -1.0,       0.0,       0.0 }, 0 }, /* -x */
    { {       0.0,      -1.0,       0.0 }, 0 }, /* -y */
    { {       0.0,       0.0,      -1.0 }, 0 }, /* -z */
    { {  SQRT_2_2,  SQRT_2_2,       0.0 }, 0 }, /* midpoint of  x and  y */
    { { -SQRT_2_2,  SQRT_2_2,       0.0 }, 0 }, /* midpoint of  y and -x */
    { { -SQRT_2_2, -SQRT_2_2,       0.0 }, 0 }, /* midpoint of -x and -y */
    { {  SQRT_2_2, -SQRT_2_2,       0.0 }, 0 }, /* midpoint of -y and  x */
    { {  SQRT_2_2,       0.0,  SQRT_2_2 }, 0 }, /* midpoint of  x and  z */
    { {       0.0,  SQRT_2_2,  SQRT_2_2 }, 0 }, /* midpoint of  y and  z */
    { { -SQRT_2_2,       0.0,  SQRT_2_2 }, 0 }, /* midpoint of -x and  z */
    { {       0.0, -SQRT_2_2,  SQRT_2_2 }, 0 }, /* midpoint of -y and  z */
    { {  SQRT_2_2,       0.0, -SQRT_2_2 }, 0 }, /* midpoint of  x and -z */
    { {       0.0,  SQRT_2_2, -SQRT_2_2 }, 0 }, /* midpoint of  y and -z */
    { { -SQRT_2_2,       0.0, -SQRT_2_2 }, 0 }, /* midpoint of -x and -z */
    { {       0.0, -SQRT_2_2, -SQRT_2_2 }, 0 }, /* midpoint of -y and -z */
    { {  SQRT_3_3,  SQRT_3_3,  SQRT_3_3 }, 0 }, /* center of N3 */
    { { -SQRT_3_3,  SQRT_3_3,  SQRT_3_3 }, 0 }, /* center of N2 */
    { { -SQRT_3_3, -SQRT_3_3,  SQRT_3_3 }, 0 }, /* center of N1 */
    { {  SQRT_3_3, -SQRT_3_3,  SQRT_3_3 }, 0 }, /* center of N0 */
    { {  SQRT_3_3,  SQRT_3_3, -SQRT_3_3 }, 0 }, /* center of S0 */
    { { -SQRT_3_3,  SQRT_3_3, -SQRT_3_3 }, 0 }, /* center of S1 */
    { { -SQRT_3_3, -SQRT_3_3, -SQRT_3_3 }, 0 }, /* center of S2 */
    { {  SQRT_3_3, -SQRT_3_3, -SQRT_3_3 }, 0 }, /* center of S3 */
    { {        C0,        C0,        C1 }, 0 }, /* center of N31 */
    { {        C1,        C0,        C0 }, 0 }, /* center of N32 */
    { {        C0,        C1,        C0 }, 0 }, /* center of N30 */
    { {       -C0,        C0,        C1 }, 0 }, /* center of N21 */
    { {       -C0,        C1,        C0 }, 0 }, /* center of N22 */
    { {       -C1,        C0,        C0 }, 0 }, /* center of N20 */
    { {       -C0,       -C0,        C1 }, 0 }, /* center of N11 */
    { {       -C1,       -C0,        C0 }, 0 }, /* center of N12 */
    { {       -C0,       -C1,        C0 }, 0 }, /* center of N10 */
    { {        C0,       -C0,        C1 }, 0 }, /* center of N01 */
    { {        C0,       -C1,        C0 }, 0 }, /* center of N02 */
    { {        C1,       -C0,        C0 }, 0 }, /* center of N00 */
    { {        C0,        C0,       -C1 }, 0 }, /* center of S01 */
    { {        C1,        C0,       -C0 }, 0 }, /* center of S00 */
    { {        C0,        C1,       -C0 }, 0 }, /* center of S02 */
    { {       -C0,        C0,       -C1 }, 0 }, /* center of S11 */
    { {       -C0,        C1,       -C0 }, 0 }, /* center of S10 */
    { {       -C1,        C0,       -C0 }, 0 }, /* center of S12 */
    { {       -C0,       -C0,       -C1 }, 0 }, /* center of S21 */
    { {       -C1,       -C0,       -C0 }, 0 }, /* center of S20 */
    { {       -C0,       -C1,       -C0 }, 0 }, /* center of S22 */
    { {        C0,       -C0,       -C1 }, 0 }, /* center of S31 */
    { {        C0,       -C1,       -C0 }, 0 }, /* center of S30 */
    { {        C1,       -C0,       -C0 }, 0 }, /* center of S32 */
};

/* Returns 1 if v (assumed to come from test_points) is a midpoint of
   two axis vectors (+/- x,y,z). These midpoints can legitimately end
   up in any of 3 children of the root triangles due to numerical
   inaccuracies. The actual child may be different depending on
   the compiler and optimization level being used. */
static int is_midpoint(const scisql_v3 *v) {
   int i0 = v->x == 0.0;
   int i1 = v->y == 0.0;
   int i2 = v->z == 0.0;
   return i0 + i1 + i2 == 1; 
}

enum {
    S0 = (SCISQL_HTM_S0 + 8), S00 = (SCISQL_HTM_S0 + 8)*4, S01, S02, S03,
    S1 = (SCISQL_HTM_S1 + 8), S10 = (SCISQL_HTM_S1 + 8)*4, S11, S12, S13,
    S2 = (SCISQL_HTM_S2 + 8), S20 = (SCISQL_HTM_S2 + 8)*4, S21, S22, S23,
    S3 = (SCISQL_HTM_S3 + 8), S30 = (SCISQL_HTM_S3 + 8)*4, S31, S32, S33,
    N0 = (SCISQL_HTM_N0 + 8), N00 = (SCISQL_HTM_N0 + 8)*4, N01, N02, N03,
    N1 = (SCISQL_HTM_N1 + 8), N10 = (SCISQL_HTM_N1 + 8)*4, N11, N12, N13,
    N2 = (SCISQL_HTM_N2 + 8), N20 = (SCISQL_HTM_N2 + 8)*4, N21, N22, N23,
    N3 = (SCISQL_HTM_N3 + 8), N30 = (SCISQL_HTM_N3 + 8)*4, N31, N32, N33
};

typedef struct {
    int64_t id;
    int nranges;
    int64_t ranges[8];
} test_results;

static const test_results level0_results[NTEST_POINTS] = {
    { /*  x */  N3, 3, { S0, S0, S3, N0, N3, N3, 0, 0 } },
    { /*  y */  N2, 2, { S0, S1, N2, N3,  0,  0, 0, 0 } },
    { /*  z */  N3, 1, { N0, N3,  0,  0,  0,  0, 0, 0 } },
    { /* -x */  N1, 2, { S1, S2, N1, N2,  0,  0, 0, 0 } },
    { /* -y */  N0, 1, { S2, N1,  0,  0,  0,  0, 0, 0 } },
    { /* -z */  S0, 1, { S0, S3,  0,  0,  0,  0, 0, 0 } },
    { /* midpoint of  x and  y */ N3, 2, { S0, S0, N3, N3, 0, 0, 0, 0 } },
    { /* midpoint of  y and -x */ N2, 2, { S1, S1, N2, N2, 0, 0, 0, 0 } },
    { /* midpoint of -x and -y */ N1, 2, { S2, S2, N1, N1, 0, 0, 0, 0 } },
    { /* midpoint of -y and  x */ N0, 1, { S3, N0,  0,  0, 0, 0, 0, 0 } },
    { /* midpoint of  x and  z */ N3, 2, { N0, N0, N3, N3, 0, 0, 0, 0 } },
    { /* midpoint of  y and  z */ N2, 1, { N2, N3,  0,  0, 0, 0, 0, 0 } },
    { /* midpoint of -x and  z */ N1, 1, { N1, N2,  0,  0, 0, 0, 0, 0 } },
    { /* midpoint of -y and  z */ N0, 1, { N0, N1,  0,  0, 0, 0, 0, 0 } },
    { /* midpoint of  x and -z */ S0, 2, { S0, S0, S3, S3, 0, 0, 0, 0 } },
    { /* midpoint of  y and -z */ S1, 1, { S0, S1,  0,  0, 0, 0, 0, 0 } },
    { /* midpoint of -x and -z */ S2, 1, { S1, S2,  0,  0, 0, 0, 0, 0 } },
    { /* midpoint of -y and -z */ S3, 1, { S2, S3,  0,  0, 0, 0, 0, 0 } },
    { /* center of N3  */ N3, 1, { N3, N3, 0, 0, 0, 0, 0, 0 } },
    { /* center of N2  */ N2, 1, { N2, N2, 0, 0, 0, 0, 0, 0 } },
    { /* center of N1  */ N1, 1, { N1, N1, 0, 0, 0, 0, 0, 0 } },
    { /* center of N0  */ N0, 1, { N0, N0, 0, 0, 0, 0, 0, 0 } },
    { /* center of S0  */ S0, 1, { S0, S0, 0, 0, 0, 0, 0, 0 } },
    { /* center of S1  */ S1, 1, { S1, S1, 0, 0, 0, 0, 0, 0 } },
    { /* center of S2  */ S2, 1, { S2, S2, 0, 0, 0, 0, 0, 0 } },
    { /* center of S3  */ S3, 1, { S3, S3, 0, 0, 0, 0, 0, 0 } },
    { /* center of N31 */ N3, 1, { N3, N3, 0, 0, 0, 0, 0, 0 } },
    { /* center of N32 */ N3, 1, { N3, N3, 0, 0, 0, 0, 0, 0 } },
    { /* center of N30 */ N3, 1, { N3, N3, 0, 0, 0, 0, 0, 0 } },
    { /* center of N21 */ N2, 1, { N2, N2, 0, 0, 0, 0, 0, 0 } },
    { /* center of N22 */ N2, 1, { N2, N2, 0, 0, 0, 0, 0, 0 } },
    { /* center of N20 */ N2, 1, { N2, N2, 0, 0, 0, 0, 0, 0 } },
    { /* center of N11 */ N1, 1, { N1, N1, 0, 0, 0, 0, 0, 0 } },
    { /* center of N12 */ N1, 1, { N1, N1, 0, 0, 0, 0, 0, 0 } },
    { /* center of N10 */ N1, 1, { N1, N1, 0, 0, 0, 0, 0, 0 } },
    { /* center of N01 */ N0, 1, { N0, N0, 0, 0, 0, 0, 0, 0 } },
    { /* center of N02 */ N0, 1, { N0, N0, 0, 0, 0, 0, 0, 0 } },
    { /* center of N00 */ N0, 1, { N0, N0, 0, 0, 0, 0, 0, 0 } },
    { /* center of S01 */ S0, 1, { S0, S0, 0, 0, 0, 0, 0, 0 } },
    { /* center of S00 */ S0, 1, { S0, S0, 0, 0, 0, 0, 0, 0 } },
    { /* center of S02 */ S0, 1, { S0, S0, 0, 0, 0, 0, 0, 0 } },
    { /* center of S11 */ S1, 1, { S1, S1, 0, 0, 0, 0, 0, 0 } },
    { /* center of S10 */ S1, 1, { S1, S1, 0, 0, 0, 0, 0, 0 } },
    { /* center of S12 */ S1, 1, { S1, S1, 0, 0, 0, 0, 0, 0 } },
    { /* center of S21 */ S2, 1, { S2, S2, 0, 0, 0, 0, 0, 0 } },
    { /* center of S20 */ S2, 1, { S2, S2, 0, 0, 0, 0, 0, 0 } },
    { /* center of S22 */ S2, 1, { S2, S2, 0, 0, 0, 0, 0, 0 } },
    { /* center of S31 */ S3, 1, { S3, S3, 0, 0, 0, 0, 0, 0 } },
    { /* center of S30 */ S3, 1, { S3, S3, 0, 0, 0, 0, 0, 0 } },
    { /* center of S32 */ S3, 1, { S3, S3, 0, 0, 0, 0, 0, 0 } }
};

static const test_results level1_results[NTEST_POINTS] = {
    { /*  x */  N32, 4, { S00, S00, S32, S32, N00, N00, N32, N32 } },
    { /*  y */  N22, 4, { S02, S02, S10, S10, N22, N22, N30, N30 } },
    { /*  z */  N31, 4, { N01, N01, N11, N11, N21, N21, N31, N31 } },
    { /* -x */  N12, 4, { S12, S12, S20, S20, N12, N12, N20, N20 } },
    { /* -y */  N02, 4, { S22, S22, S30, S30, N02, N02, N10, N10 } },
    { /* -z */  S01, 4, { S01, S01, S11, S11, S21, S21, S31, S31 } },
    { /* midpoint of  x and  y */ 0, 4, { S00, S00, S02, S03, N30, N30, N32, N33 } },
    { /* midpoint of  y and -x */ 0, 4, { S10, S10, S12, S13, N20, N20, N22, N23 } },
    { /* midpoint of -x and -y */ 0, 4, { S20, S20, S22, S23, N10, N10, N12, N13 } },
    { /* midpoint of -y and  x */ 0, 3, { S30, S30, S32, N00, N02, N03, 0, 0 } },
    { /* midpoint of  x and  z */ 0, 3, { N00, N01, N03, N03, N31, N33, 0, 0 } },
    { /* midpoint of  y and  z */ 0, 2, { N21, N31, N33, N33,   0,   0, 0, 0 } },
    { /* midpoint of -x and  z */ 0, 2, { N11, N21, N23, N23,   0,   0, 0, 0 } },
    { /* midpoint of -y and  z */ 0, 2, { N01, N11, N13, N13,   0,   0, 0, 0 } },
    { /* midpoint of  x and -z */ 0, 3, { S00, S01, S03, S03, S31, S33, 0, 0 } },
    { /* midpoint of  y and -z */ 0, 2, { S01, S11, S13, S13,   0,   0, 0, 0 } },
    { /* midpoint of -x and -z */ 0, 2, { S11, S21, S23, S23,   0,   0, 0, 0 } },
    { /* midpoint of -y and -z */ 0, 2, { S21, S31, S33, S33,   0,   0, 0, 0 } },
    { /* center of N3  */ N33, 1, { N33, N33, 0, 0, 0, 0, 0, 0 } },
    { /* center of N2  */ N23, 1, { N23, N23, 0, 0, 0, 0, 0, 0 } },
    { /* center of N1  */ N13, 1, { N13, N13, 0, 0, 0, 0, 0, 0 } },
    { /* center of N0  */ N03, 1, { N03, N03, 0, 0, 0, 0, 0, 0 } },
    { /* center of S0  */ S03, 1, { S03, S03, 0, 0, 0, 0, 0, 0 } },
    { /* center of S1  */ S13, 1, { S13, S13, 0, 0, 0, 0, 0, 0 } },
    { /* center of S2  */ S23, 1, { S23, S23, 0, 0, 0, 0, 0, 0 } },
    { /* center of S3  */ S33, 1, { S33, S33, 0, 0, 0, 0, 0, 0 } },
    { /* center of N31 */ N31, 1, { N31, N31, 0, 0, 0, 0, 0, 0 } },
    { /* center of N32 */ N32, 1, { N32, N32, 0, 0, 0, 0, 0, 0 } },
    { /* center of N30 */ N30, 1, { N30, N30, 0, 0, 0, 0, 0, 0 } },
    { /* center of N21 */ N21, 1, { N21, N21, 0, 0, 0, 0, 0, 0 } },
    { /* center of N22 */ N22, 1, { N22, N22, 0, 0, 0, 0, 0, 0 } },
    { /* center of N20 */ N20, 1, { N20, N20, 0, 0, 0, 0, 0, 0 } },
    { /* center of N11 */ N11, 1, { N11, N11, 0, 0, 0, 0, 0, 0 } },
    { /* center of N12 */ N12, 1, { N12, N12, 0, 0, 0, 0, 0, 0 } },
    { /* center of N10 */ N10, 1, { N10, N10, 0, 0, 0, 0, 0, 0 } },
    { /* center of N01 */ N01, 1, { N01, N01, 0, 0, 0, 0, 0, 0 } },
    { /* center of N02 */ N02, 1, { N02, N02, 0, 0, 0, 0, 0, 0 } },
    { /* center of N00 */ N00, 1, { N00, N00, 0, 0, 0, 0, 0, 0 } },
    { /* center of S01 */ S01, 1, { S01, S01, 0, 0, 0, 0, 0, 0 } },
    { /* center of S00 */ S00, 1, { S00, S00, 0, 0, 0, 0, 0, 0 } },
    { /* center of S02 */ S02, 1, { S02, S02, 0, 0, 0, 0, 0, 0 } },
    { /* center of S11 */ S11, 1, { S11, S11, 0, 0, 0, 0, 0, 0 } },
    { /* center of S10 */ S10, 1, { S10, S10, 0, 0, 0, 0, 0, 0 } },
    { /* center of S12 */ S12, 1, { S12, S12, 0, 0, 0, 0, 0, 0 } },
    { /* center of S21 */ S21, 1, { S21, S21, 0, 0, 0, 0, 0, 0 } },
    { /* center of S20 */ S20, 1, { S20, S20, 0, 0, 0, 0, 0, 0 } },
    { /* center of S22 */ S22, 1, { S22, S22, 0, 0, 0, 0, 0, 0 } },
    { /* center of S31 */ S31, 1, { S31, S31, 0, 0, 0, 0, 0, 0 } },
    { /* center of S30 */ S30, 1, { S30, S30, 0, 0, 0, 0, 0, 0 } },
    { /* center of S32 */ S32, 1, { S32, S32, 0, 0, 0, 0, 0, 0 } }
};

static const test_results * const results[2] = {
    level0_results, level1_results
};


/*  Tests HTM indexing routines on predefined points.
 */
static void testPoints() {
    scisql_v3p pts[NTEST_POINTS];
    int64_t ids[NTEST_POINTS];
    size_t i, j, k;
    int ret;
    int level;

    memset(pts, 0, sizeof(pts));
    memset(ids, 0, sizeof(ids));

    /* Failure tests */
    SCISQL_ASSERT(scisql_v3_htmid(0, 0) == -1,
                  "scisql_v3_htmid() should have failed");
    SCISQL_ASSERT(scisql_v3_htmid(&pts[0].v, -1) == -1,
                  "scisql_v3_htmid() should have failed");
    SCISQL_ASSERT(scisql_v3_htmid(&pts[0].v, SCISQL_HTM_MAX_LEVEL + 1) == -1,
                  "scisql_v3_htmid() should have failed");
    SCISQL_ASSERT(scisql_v3p_htmsort(0, ids, 1, 0) != 0,
                  "scisql_v3p_htmsort() should have failed");
    SCISQL_ASSERT(scisql_v3p_htmsort(pts, 0, 1, 0) != 0,
                  "scisql_v3p_htmsort() should have failed");
    SCISQL_ASSERT(scisql_v3p_htmsort(pts, ids, 1, -1) != 0,
                  "scisql_v3p_htmsort() should have failed");
    SCISQL_ASSERT(scisql_v3p_htmsort(pts, ids, 1, SCISQL_HTM_MAX_LEVEL + 1) != 0,
                  "scisql_v3p_htmsort() should have failed");

    for (level = 0; level < 2; ++level) {
        memcpy(pts, test_points, sizeof(test_points));
        ret = scisql_v3p_htmsort(pts, ids, NTEST_POINTS, level);
        SCISQL_ASSERT(ret == 0, "scisql_v3p_htmsort() failed");
        for (i = 0; i < NTEST_POINTS; ++i) {
            int64_t id = scisql_v3_htmid(&pts[i].v, level);
            if (id != ids[i]) {
                SCISQL_ASSERT(level > 0 && is_midpoint(&pts[i].v),
                              "scisql_v3p_htmsort() does not agree "
                              "with scisql_v3_htmid()");
            }
            SCISQL_ASSERT(scisql_htm_level(id) == level,
                          "scisql_htm_level() failed");
        }
        for (i = 0; i < NTEST_POINTS; ++i) {
            int64_t eid = results[level][i].id;
            int64_t id = scisql_v3_htmid(&test_points[i].v, level);
            SCISQL_ASSERT(eid == id || eid == 0, "scisql_v3_htmid() "
                          "did not produce expected result");
        }
        /* Test scisql_v3p_htmsort() on arrays of varying length */
        for (i = 1; i < NTEST_POINTS; ++i) {
            for (j = 0; j <= NTEST_POINTS - i; ++j) {
                memcpy(pts, test_points + j, i * sizeof(scisql_v3p));
                ret = scisql_v3p_htmsort(pts, ids, i, level);
                SCISQL_ASSERT(ret == 0, "scisql_v3p_htmsort() failed");
                for (k = 0; k < i; ++k) {
                    int64_t id = scisql_v3_htmid(&pts[k].v, level);
                    if (id != ids[k]) {
                        SCISQL_ASSERT(level > 0 && is_midpoint(&pts[k].v),
                                      "scisql_v3p_htmsort() does not agree "
                                      "with scisql_v3_htmid()");
                    }
                    SCISQL_ASSERT(scisql_htm_level(id) == level,
                                  "scisql_htm_level() failed");
                }
            }
        }
    }
    /* Tests at subdivision levels 2 to SCISQL_HTM_MAX_LEVEL */
    for (i = CENTERS; i < NTEST_POINTS; ++i) {
        ids[i] = level1_results[i].id;
    }
    for (level = 2; level <= SCISQL_HTM_MAX_LEVEL; ++level) {
        size_t n = (level < 8) ? NTEST_POINTS : CENTERS + 8;
        for (i = CENTERS; i < n; ++i) {
            int64_t id = scisql_v3_htmid(&test_points[i].v, level);
            int64_t eid = ids[i] * 4 + 3;
            SCISQL_ASSERT(id == eid, "scisql_v3_htmid() did not produce "
                          "expected result (L%d, pt %d)", level, (int)i);
            ids[i] = eid;
            SCISQL_ASSERT(scisql_htm_level(id) == level,
                          "scisql_htm_level() failed");
        }
    }
}


/*  Tests that scisql_v3_htmid and scisql_v3p_htmsort agree for large
    arrays of random points.
 */
static void testRandomPoints() {
    scisql_v3p *pts;
    int64_t *ids;
    size_t i;
    int level, ret;
    const size_t n = 10000;
    unsigned short seed[3] = { 11, 21, 31 };

    pts = malloc(sizeof(scisql_v3p) * n);
    SCISQL_ASSERT(pts != 0, "memory allocation failed");
    ids = malloc(sizeof(int64_t) * n);
    SCISQL_ASSERT(ids != 0, "memory allocation failed");
    for (level = 0; level <= SCISQL_HTM_MAX_LEVEL; ++level) {
        for (i = 0; i < n; ++i) {
            pts[i].v.x = erand48(seed) - 0.5;
            pts[i].v.y = erand48(seed) - 0.5;
            pts[i].v.z = erand48(seed) - 0.5;
            scisql_v3_normalize(&pts[i].v, &pts[i].v);
        }
        ret = scisql_v3p_htmsort(pts, ids, n, level);
        SCISQL_ASSERT(ret == 0, "scisql_v3p_htmsort() failed");
        for (i = 0; i < n; ++i) {
            int64_t id = scisql_v3_htmid(&pts[i].v, level);
            SCISQL_ASSERT(id == ids[i], "scisql_v3_htmid() does not agree "
                          "with scisql_v3p_htmsort()");
            SCISQL_ASSERT(scisql_htm_level(id) == level,
                          "scisql_htm_level() failed");
        }
    }
    free(ids);
    free(pts);
}


/*  Tests HTM indexing of spherical circles.
 */
static void testCircles() {
    scisql_ids *ids = 0;
    const scisql_v3 *v = &test_points[0].v;
    double radius = 10.0;
    int i, j, level;

    /* Failure tests */
    SCISQL_ASSERT(scisql_s2circle_htmids(0, 0, 0.0, 0, SIZE_MAX) == 0,
                  "scisql_s2circle_htmids() should have failed");
    SCISQL_ASSERT(scisql_s2circle_htmids(0, v, 0.0, -1, SIZE_MAX) == 0,
                  "scisql_s2circle_htmids() should have failed");
    SCISQL_ASSERT(scisql_s2circle_htmids(0, v, 0.0, SCISQL_HTM_MAX_LEVEL + 1, SIZE_MAX) == 0,
                  "scisql_s2circle_htmids() should have failed");

    for (level = 0; level < 2; ++level) {
        for (i = 0; i < NTEST_POINTS; ++i) {
            int nr = results[level][i].nranges;
            ids = scisql_s2circle_htmids(ids, &test_points[i].v, radius, level, SIZE_MAX);
            SCISQL_ASSERT(ids != 0, "scisql_s2circle_htmids() failed");
            SCISQL_ASSERT(ids->n == (size_t) nr,
                          "scisql_s2circle_htmids() did not return the "
                          "expected number of ranges");
            for (j = 0; j < 2 * nr; ++j) {
                SCISQL_ASSERT(results[level][i].ranges[j] == ids->ranges[j],
                              "scisql_s2circle_htmids() did not return the "
                              "expected ranges");
            }
        }
    }
    /* Tests at subdivision levels 2 to SCISQL_HTM_MAX_LEVEL */
    radius = 1.0;
    for (level = 2; level < 8; ++level, radius *= 0.5) {
        int n = (level < 8) ? NTEST_POINTS : CENTERS + 8;
        for (i = CENTERS; i < n; ++i) {
            int64_t id = scisql_v3_htmid(&test_points[i].v, level);
            ids = scisql_s2circle_htmids(ids, &test_points[i].v, radius, level, SIZE_MAX);
            SCISQL_ASSERT(ids != 0, "scisql_s2circle_htmids() failed");
            SCISQL_ASSERT(ids->n == 1,
                          "scisql_s2circle_htmids() did not return the "
                          "expected number of ranges");
            SCISQL_ASSERT(id == ids->ranges[0] && id == ids->ranges[1],
                          "scisql_s2circle_htmids() did not return the "
                          "expected ranges");
        }
    }
    free(ids);
}


/*  Utility function to build an N-gon inscribed in the given circle.
 */
static int ngon(scisql_s2cpoly *poly,
                int n,
                const scisql_v3 *center,
                double radius)
{
    scisql_v3 verts[SCISQL_MAX_VERTS];
    scisql_v3 north, east, v;
    double sr, cr;
    int i;

    if (n < 3 || n > SCISQL_MAX_VERTS || center == 0 || radius <= 0.0) {
        return 1;
    }

    north.x = - center->x * center->z;
    north.y = - center->y * center->z;
    north.z = center->x * center->x + center->y * center->y;
    if (north.x == 0.0 && north.y == 0.0 && north.z == 0.0) {
        north.x = -1.0; north.y = 0.0; north.z = 0.0;
        east.x = 0.0; east.y = 1.0; east.z = 0.0;
    } else {
        scisql_v3_rcross(&east, &north, center);
        scisql_v3_normalize(&north, &north);
        scisql_v3_normalize(&east, &east);
    }
    sr = sin(radius * SCISQL_RAD_PER_DEG);
    cr = cos(radius * SCISQL_RAD_PER_DEG);
    for (i = 0; i < n; ++i) {
        double ang, sa, ca;
        ang = (SCISQL_RAD_PER_DEG * 360.0 * i) / n;
        sa = sin(ang);
        ca = cos(ang);
        v.x = ca * north.x + sa * east.x;
        v.y = ca * north.y + sa * east.y;
        v.z = ca * north.z + sa * east.z;
        verts[i].x = cr * center->x + sr * v.x;
        verts[i].y = cr * center->y + sr * v.y;
        verts[i].z = cr * center->z + sr * v.z;
        scisql_v3_normalize(&verts[i], &verts[i]);
    }
    return scisql_s2cpoly_init(poly, verts, n);
}


/*  Tests HTM indexing of spherical convex polygons.
 */
static void testPolygons() {
    scisql_v3 sliver[3];
    scisql_sc p;
    scisql_s2cpoly poly;
    scisql_ids *ids = 0;
    double radius = 10.0;
    int i, j, level;

    memset(&poly, 0, sizeof(poly));

    /* Failure tests */
    SCISQL_ASSERT(scisql_s2cpoly_htmids(0, 0, 0, SIZE_MAX) == 0,
                  "scisql_s2cpoly_htmids() should have failed");
    SCISQL_ASSERT(scisql_s2cpoly_htmids(0, &poly, -1, SIZE_MAX) == 0,
                  "scisql_s2cpoly_htmids() should have failed");
    SCISQL_ASSERT(scisql_s2cpoly_htmids(0, &poly, SCISQL_HTM_MAX_LEVEL + 1, SIZE_MAX) == 0,
                  "scisql_s2cpoly_htmids() should have failed");

    for (level = 0; level < 2; ++level) {
        for (i = 0; i < NTEST_POINTS; ++i) {
            int nr = results[level][i].nranges;
            int ret = ngon(&poly, 4, &test_points[i].v, radius);
            SCISQL_ASSERT(ret == 0, "ngon() failed");
            ids = scisql_s2cpoly_htmids(ids, &poly, level, SIZE_MAX);
            SCISQL_ASSERT(ids != 0, "scisql_s2cpoly_htmids() failed");
            SCISQL_ASSERT(ids->n == (size_t) nr,
                          "scisql_s2cpoly_htmids() did not return the "
                          "expected number of ranges");
            for (j = 0; j < 2 * nr; ++j) {
                SCISQL_ASSERT(results[level][i].ranges[j] == ids->ranges[j],
                              "scisql_s2cpoly_htmids() did not return the "
                              "expected ranges");
            }
        }
    }
    /* Tests at subdivision levels 2 to SCISQL_HTM_MAX_LEVEL */
    radius = 1.0;
    for (level = 2; level < 8; ++level, radius *= 0.5) {
        int n = (level < 8) ? NTEST_POINTS : CENTERS + 8;
        for (i = CENTERS; i < n; ++i) {
            int64_t id = scisql_v3_htmid(&test_points[i].v, level);
            int ret = ngon(&poly, 4, &test_points[i].v, radius);
            SCISQL_ASSERT(ret == 0, "ngon() failed");
            ids = scisql_s2cpoly_htmids(ids, &poly, level, SIZE_MAX);
            SCISQL_ASSERT(ids != 0, "scisql_s2cpoly_htmids() failed");
            SCISQL_ASSERT(ids->n == 1,
                          "scisql_s2cpoly_htmids() did not return the "
                          "expected number of ranges");
            SCISQL_ASSERT(id == ids->ranges[0] && id == ids->ranges[1],
                          "scisql_s2cpoly_htmids() did not return the "
                          "expected ranges");
        }
    }
    scisql_sc_init(&p, 1.0, -1.0);
    scisql_sctov3(&sliver[0], &p);
    scisql_sc_init(&p, 359.0, 4.0);
    scisql_sctov3(&sliver[1], &p);
    scisql_sc_init(&p, 358.0, 3.0);
    scisql_sctov3(&sliver[2], &p);
    scisql_s2cpoly_init(&poly, sliver, 3);
    ids = scisql_s2cpoly_htmids(ids, &poly, 0, SIZE_MAX);
    SCISQL_ASSERT(ids->n == 3,
                  "scisql_s2cpoly_htmids() did not return the "
                  "expected number of ranges");
    SCISQL_ASSERT(ids->ranges[0] == S0 && ids->ranges[1] == S0 &&
                  ids->ranges[2] == N0 && ids->ranges[3] == N0 &&
                  ids->ranges[4] == N3 && ids->ranges[5] == N3,
                  "scisql_s2cpoly_htmids() did not return the "
                  "expected ranges");
    ids = scisql_s2cpoly_htmids(ids, &poly, 1, SIZE_MAX);
    SCISQL_ASSERT(ids->n == 3,
                  "scisql_s2cpoly_htmids() did not return the "
                  "expected number of ranges");
    SCISQL_ASSERT(ids->ranges[0] == S00 && ids->ranges[1] == S00 &&
                  ids->ranges[2] == N00 && ids->ranges[3] == N00 &&
                  ids->ranges[4] == N32 && ids->ranges[5] == N32,
                  "scisql_s2cpoly_htmids() did not return the "
                  "expected ranges");
    free(ids);
}


/*  Checks that ID range list a is a subset of b.
 */
static void checkSubset(scisql_ids const *a, scisql_ids const *b) {
    size_t i, j;
    for (i = 0, j = 0; i < a->n && j < b->n;) {
        if (a->ranges[2*i] > b->ranges[2*j + 1]) {
            ++j;
            continue;
        }
        SCISQL_ASSERT(a->ranges[2*i] >= b->ranges[2*j],
                      "fine range list is not a subset of coarse range list");
        SCISQL_ASSERT(a->ranges[2*i + 1] <= b->ranges[2*j + 1],
                      "fine range list is not a subset of coarse range list");
        ++i;
    }
    SCISQL_ASSERT(i == a->n,
                  "fine range list is not a subset of coarse range list");
    SCISQL_ASSERT(j == b->n - 1,
                  "coarse range list includes unnecessary ranges");
}


/*  Tests adaptive coarsening of effective subdivision level with circles.
 */
static void testAdaptiveCircle() {
    static const double radii[3] = { 0.001, 0.1, 10.0 };
    scisql_v3 center = test_points[18].v;
    scisql_ids *coarse = 0;
    scisql_ids *fine = 0;
    int i, level;
    for (i = 0; i < 3; ++i) {
        for (level = 0; level <= SCISQL_HTM_MAX_LEVEL; ++level) {
            fine = scisql_s2circle_htmids(fine, &center, radii[i], level, SIZE_MAX);
            SCISQL_ASSERT(fine != 0, "scisql_s2cpoly_htmids() failed");
            coarse = scisql_s2circle_htmids(coarse, &center, radii[i], level, 16);
            SCISQL_ASSERT(coarse != 0, "scisql_s2cpoly_htmids() failed");
            checkSubset(fine, coarse); 
       }
    }
    free(coarse);
    free(fine);
}


/*  Tests adaptive coarsening of effective subdivision level with polygons.
 */
static void testAdaptivePoly() {
    static const double radii[3] = { 0.001, 0.1, 1.0 };
    scisql_s2cpoly poly;
    scisql_v3 center = test_points[18].v;
    scisql_ids *coarse = 0;
    scisql_ids *fine = 0;
    int i, level;
    for (i = 0; i < 3; ++i) {
        int ret = ngon(&poly, 4, &center, radii[i]);
        SCISQL_ASSERT(ret == 0, "ngon() failed");
        for (level = 0; level <= SCISQL_HTM_MAX_LEVEL; ++level) {
            fine = scisql_s2cpoly_htmids(fine, &poly, level, SIZE_MAX);
            SCISQL_ASSERT(fine != 0, "scisql_s2cpoly_htmids() failed");
            coarse = scisql_s2cpoly_htmids(coarse, &poly, level, 16);
            SCISQL_ASSERT(coarse != 0, "scisql_s2cpoly_htmids() failed");
            checkSubset(fine, coarse);
       }
    }
    free(coarse);
    free(fine);
}


int main(int argc SCISQL_UNUSED, char **argv SCISQL_UNUSED) {
    testPoints();
    testRandomPoints();
    testCircles();
    testPolygons();
    testAdaptiveCircle();
    testAdaptivePoly();
    return 0;
}


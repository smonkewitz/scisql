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

    A minimalistic set of functions and types for spherical geometry.
*/

#ifndef SCISQL_GEOMETRY_H
#define SCISQL_GEOMETRY_H
#include <ctype.h>
#include <math.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


#define SCISQL_DEG_PER_RAD    57.2957795130823208767981548141
#define SCISQL_RAD_PER_DEG    0.0174532925199432957692369076849
#define SCISQL_ARCSEC_PER_DEG 3600.0


/* ---- Utilities ---- */

/*  Returns the given angle, range-reduced to lie in [0, 360) degrees.
 */
SCISQL_INLINE double scisql_angred(double angle_deg) {
    double angle = fmod(angle_deg, 360.0);
    if (angle < 0.0) {
        angle += 360.0;
        if (angle == 360.0) {
            angle = 0.0;
        }
    }
    return angle;
}

/*  Returns the given value, clamped to lie in <tt>[min, max]</tt>.
 */
SCISQL_INLINE double scisql_clamp(double x, double min, double max) {
    return (x < min) ? min : ((x > max) ? max : x);
}


/* ---- Spherical coordinates and 3-vectors ---- */

/*  Cartesian coordinates for a point in R3.
 */
typedef struct {
    double x;
    double y;
    double z;
} scisql_v3;

/*  Spherical coordinates (in degrees) for a point in S2.
 */
typedef struct {
    double lon;
    double lat;
} scisql_sc;

/*  Stores the given coordinates in out, which must not be null. Returns 1
    if any input coordinate is non-finite, and 0 on success.
 */
SCISQL_INLINE int scisql_v3_init(scisql_v3 *out, double x, double y, double z) {
    if (SCISQL_ISSPECIAL(x) || SCISQL_ISSPECIAL(y) || SCISQL_ISSPECIAL(z)) {
        return 1;
    }
    out->x = x;
    out->y = y;
    out->z = z;
    return 0;
}

/*  Stores the given coordinates in out, which must not be null. Returns 1
    if any input coordinate is non-finite or if lat is not in the [-90, 90]
    degree range, and 0 on success.
 */
SCISQL_INLINE int scisql_sc_init(scisql_sc *out, double lon, double lat) {
    if (SCISQL_ISSPECIAL(lon) || SCISQL_ISSPECIAL(lat) || lat < -90.0 || lat > 90.0) {
        return 1;
    }
    out->lon = lon;
    out->lat = lat;
    return 0;
}

/*  Stores the vector sum v1 + v2 in out.  Arguments must not be null
    pointers, but may alias.
 */
SCISQL_INLINE void scisql_v3_add(scisql_v3 *out,
                                 const scisql_v3 *v1,
                                 const scisql_v3 *v2)
{
    out->x = v1->x + v2->x;
    out->y = v1->y + v2->y;
    out->z = v1->z + v2->z;
}

/*  Stores the vector difference v1 - v2 in out.  Arguments must not be
    null pointers, but may alias.
 */
SCISQL_INLINE void scisql_v3_sub(scisql_v3 *out,
                                 const scisql_v3 *v1,
                                 const scisql_v3 *v2)
{
    out->x = v1->x - v2->x;
    out->y = v1->y - v2->y;
    out->z = v1->z - v2->z;
}

/*  Stores the vector v * -1 in out.  Arguments must not be null pointers,
    but may alias.
 */
SCISQL_INLINE void scisql_v3_neg(scisql_v3 *out, const scisql_v3 *v) {
    out->x = - v->x;
    out->y = - v->y;
    out->z = - v->z;
}

/*  Stores the vector-scalar product v * s in out.  Arguments must not be
    null pointers, but may alias.
 */
SCISQL_INLINE void scisql_v3_mul(scisql_v3 *out, const scisql_v3 *v, double s) {
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}

/*  Stores the vector-scalar quotient v / s in out.  Arguments must not be
    null pointers, but may alias.
 */
SCISQL_INLINE void scisql_v3_div(scisql_v3 *out, const scisql_v3 *v, double s) {
    out->x = v->x / s;
    out->y = v->y / s;
    out->z = v->z / s;
}

/*  Returns the dot product of the 3-vectors v1 and v2.  Arguments must not be
    null pointers, but may alias.
 */
SCISQL_INLINE double scisql_v3_dot(const scisql_v3 *v1, const scisql_v3 *v2) {
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

/*  Returns the squared norm of the 3-vector v, which must not be null.
    Equivalent to scisql_v3_dot(v, v).
 */
SCISQL_INLINE double scisql_v3_norm2(const scisql_v3 *v) {
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

/*  Returns the norm of the 3-vector v, which must not be null.
 */
SCISQL_INLINE double scisql_v3_norm(const scisql_v3 *v) {
    return sqrt(scisql_v3_norm2(v));
}

/*  Stores a normalized copy of v in out.  Arguments must not be
    null pointers, but may alias.
  */
SCISQL_INLINE void scisql_v3_normalize(scisql_v3 *out, const scisql_v3 *v) {
    double norm = scisql_v3_norm(v);
    out->x = v->x / norm;
    out->y = v->y / norm;
    out->z = v->z / norm;
}

/*  Stores twice the vector cross product of v1 and v2 in out.  Arguments must
    not be null pointers, but may alias.
 */
SCISQL_INLINE void scisql_v3_rcross(scisql_v3 *out,
                                    const scisql_v3 *v1,
                                    const scisql_v3 *v2)
{
    double x1 = v2->x + v1->x;
    double x2 = v2->x - v1->x;
    double y1 = v2->y + v1->y;
    double y2 = v2->y - v1->y;
    double z1 = v2->z + v1->z;
    double z2 = v2->z - v1->z;
    out->x = y1 * z2 - z1 * y2;
    out->y = z1 * x2 - x1 * z2;
    out->z = x1 * y2 - y1 * x2;
}

/*  Stores the vector cross product of v1 and v2 in out.  Arguments must not be
    null pointers, but may alias.
 */
SCISQL_INLINE void scisql_v3_cross(scisql_v3 *out,
                                   const scisql_v3 *v1,
                                   const scisql_v3 *v2)
{
    double x = v1->y * v2->z - v1->z * v2->y;
    double y = v1->z * v2->x - v1->x * v2->z;
    double z = v1->x * v2->y - v1->y * v2->x;
    out->x = x;
    out->y = y;
    out->z = z;
}

/*  Converts the spherical coordinate pair p to a unit 3-vector and stores
    the results in out.  Arguments must not be null pointers.
 */
SCISQL_LOCAL void scisql_sctov3(scisql_v3 *out, const scisql_sc *p);

/*  Converts the 3-vector v to spherical coordinates and stores the results
    in out.  The vector v is not required to have unit norm.  Argument
    pointers must be non-null.
 */
SCISQL_LOCAL void scisql_v3tosc(scisql_sc *out, const scisql_v3 *v);


/* ---- Angular separation and distance ---- */

/*  Returns the square of the distance between the unit vectors corresponding
    to points p1 and p2.  Arguments must not be null pointers, but may alias.
 */
SCISQL_LOCAL double scisql_sc_dist2(const scisql_sc *p1, const scisql_sc *p2);

/*  Returns the angular separation (in degrees) between the points p1 and
    p2.  Arguments must not not be null pointers, but may alias.
 */
SCISQL_LOCAL double scisql_sc_angsep(const scisql_sc *p1, const scisql_sc *p2);

/*  Returns the square of the distance betwen vectors v1 and v2.
    Arguments must not be null pointers, but may alias.
 */
SCISQL_INLINE double scisql_v3_dist2(const scisql_v3 *v1, const scisql_v3 *v2) {
    scisql_v3 delta;
    scisql_v3_sub(&delta, v1, v2);
    return scisql_v3_norm2(&delta);
}

/*  Returns the angular separation (in degrees) between unit vectors
    unit_v1 and unit_v2.  Arguments must not be null pointers, but may alias.
 */
SCISQL_LOCAL double scisql_v3_angsepu(const scisql_v3 *unit_v1,
                                      const scisql_v3 *unit_v2);

/*  Returns the angular separation (in degrees) between vectors v1 and
    v2, which need not have unit norm.  Arguments must not be null pointers,
    but may alias.
 */
SCISQL_LOCAL double scisql_v3_angsep(const scisql_v3 *v1, const scisql_v3 *v2);

/*  Returns the minimum square distance between v, and points on the edge
    from v1 to v2 (where e is a vector parallel to the cross product of
    v1 and v2). The vectors v, v1, and v2 are assumed to be normalized,
    e need not have unit norm.
 */
SCISQL_LOCAL double scisql_v3_edgedist2(const scisql_v3 *v,
                                        const scisql_v3 *v1,
                                        const scisql_v3 *v2,
                                        const scisql_v3 *e);


/* ---- Convex Spherical Polygons ---- */

#define SCISQL_MAX_VERTS 20

/** A convex polygon on the sphere.
  */
typedef struct {
    size_t n; /* number of edges (and vertices). */
    scisql_v3 vsum; /* sum of all vertices in polygon. */
    scisql_v3 edges[SCISQL_MAX_VERTS];
} scisql_s2cpoly;

/*  Initializes a scisql_s2cpoly from a list of between 3 and
    SCISQL_MAX_VERTS vertices. Vertices can be in clockwise or
    counter-clockwise order, but are assumed to be hemispherical, to
    define edges that do not intersect except at vertices, and to define
    edges forming a convex polygon.

    Returns 0 on success and 1 on error.
  */
SCISQL_LOCAL int scisql_s2cpoly_init(scisql_s2cpoly *out,
                                     const scisql_v3 *verts,
                                     size_t n);

/*  Initializes a scisql_s2cpoly from a byte string representation.

    Returns 0 on success and 1 on error.
  */
SCISQL_LOCAL int scisql_s2cpoly_frombin(scisql_s2cpoly *out,
                                        const unsigned char *s,
                                        size_t len);

/*  Returns 1 if the spherical convex polygon cp contains
    vector v, and 0 otherwise.
  */
SCISQL_LOCAL int scisql_s2cpoly_cv3(const scisql_s2cpoly *cp,
                                    const scisql_v3 *v);

/*  Returns a byte string representation of a scisql_s2cpoly. For a polygon
    with N vertices (and edges), 3*sizeof(double)*(N + 1) bytes of storage are
    required.

    Returns the number of bytes written. This will be zero if out
    or cp is null, or if len is too small.
  */
SCISQL_LOCAL size_t scisql_s2cpoly_tobin(unsigned char *out,
                                         size_t len,
                                         const scisql_s2cpoly *cp);


#ifdef __cplusplus
}
#endif

#endif /* SCISQL_GEOMETRY_H */


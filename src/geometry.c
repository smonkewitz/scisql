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

    Spherical geometry library implementation. For documentation,
    see "geometry.h".
 */
#include "geometry.h"

#include <endian.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


/* ---- Spherical coordinates and 3-vectors ---- */

SCISQL_LOCAL void scisql_sctov3(scisql_v3 *out, const scisql_sc *p) {
    double lon = p->lon * SCISQL_RAD_PER_DEG;
    double lat = p->lat * SCISQL_RAD_PER_DEG;
    double cos_lat = cos(lat);
    out->x = cos(lon) * cos_lat;
    out->y = sin(lon) * cos_lat;
    out->z = sin(lat);
}


SCISQL_LOCAL void scisql_v3tosc(scisql_sc *out, const scisql_v3 *v) {
    double d2 = v->x*v->x + v->y*v->y;
    if (d2 == 0.0) {
        out->lon = 0.0;
    } else {
        double lon = atan2(v->y, v->x) * SCISQL_DEG_PER_RAD;
        if (lon < 0.0) {
            lon += 360.0;
            if (lon == 360.0) {
                lon = 0.0;
            }
        }
        out->lon = lon;
    }
    if (v->z == 0.0) {
        out->lat = 0.0;
    } else {
        out->lat = scisql_clamp(atan2(v->z, sqrt(d2)) * SCISQL_DEG_PER_RAD,
                                -90.0, 90.0);
    }
}


/* ---- Angular separation and distance ---- */

SCISQL_LOCAL double scisql_sc_dist2(const scisql_sc *p1, const scisql_sc *p2) {
    double x, y, z, d2;
    x = sin((p1->lon - p2->lon) * SCISQL_RAD_PER_DEG * 0.5);
    x *= x;
    y = sin((p1->lat - p2->lat) * SCISQL_RAD_PER_DEG * 0.5);
    y *= y;
    z = cos((p1->lat + p2->lat) * SCISQL_RAD_PER_DEG * 0.5);
    z *= z;
    d2 = 4.0 * (x * (z - y) + y);
    return d2 < 0.0 ? 0.0 : (d2 > 4.0 ? 4.0 : d2);
}


SCISQL_LOCAL double scisql_sc_angsep(const scisql_sc *p1, const scisql_sc *p2) {
    double angsep, x;
    x = scisql_sc_dist2(p1, p2) * 0.25;
    angsep = 2.0 * SCISQL_DEG_PER_RAD * asin(sqrt(x));
    return angsep > 180.0 ? 180.0 : angsep;
}


SCISQL_LOCAL double scisql_v3_angsepu(const scisql_v3 *unit_v1,
                                      const scisql_v3 *unit_v2)
{
    double angsep, x;
    x = scisql_v3_dist2(unit_v1, unit_v2) * 0.25;
    angsep = 2.0 * SCISQL_DEG_PER_RAD * asin(sqrt(x > 1.0 ? 1.0 : x));
    return angsep > 180.0 ? 180.0 : angsep;
}


SCISQL_LOCAL double scisql_v3_angsep(const scisql_v3 *v1, const scisql_v3 *v2) {
    scisql_v3 n;
    double ss, cs, angsep;
    scisql_v3_cross(&n, v1, v2);
    ss = scisql_v3_norm(&n);
    cs = scisql_v3_dot(v1, v2);
    if (cs == 0.0 && ss == 0.0) {
        return 0.0;
    }
    angsep = atan2(ss, cs) * SCISQL_DEG_PER_RAD;
    return (angsep > 180.0) ? 180.0 : angsep;
}


/* ---- Convex Spherical Polygons ---- */

typedef union {
    double value;
    unsigned char bytes[sizeof(double)];
} _scisql_double_bytes;


SCISQL_LOCAL int scisql_s2cpoly_init(scisql_s2cpoly *out,
                                     const scisql_v3 *verts,
                                     size_t n)
{
    size_t i;
    if (out == 0 || verts == 0 || n < 3 || n > SCISQL_MAX_VERTS) {
        return 1;
    }
    out->n = n;
    for (i = 0; i < n - 1; ++i) {
        /* the cross product two consecutive vertices gives a vector
           parallel to the edge plane normal. */
        scisql_v3_cross(&out->edges[i], &verts[i], &verts[i + 1]);
    }
    /* compute last edge plane */
    scisql_v3_cross(&out->edges[n - 1], &verts[n - 1], &verts[0]);
    return 0;
}


SCISQL_LOCAL int scisql_s2cpoly_cv3(const scisql_s2cpoly *cp,
                                    const scisql_v3 *v)
{
    size_t i;
    for (i = 0; i < cp->n; ++i) {
        if (scisql_v3_dot(v, &cp->edges[i]) < 0.0) {
            return 0;
        }
    }
    return 1;
}


#if __BYTE_ORDER == __LITTLE_ENDIAN
#   warning    Byte order detected as little endian
#   define SCISQL_COPY_DBL_BYTES(dst, src) \
    do { memcpy((dst), (src), sizeof(double)); } while(0)
#elif __BYTE_ORDER == __BIG_ENDIAN
#   warning    Byte order detected as big endian
#   define SCISQL_COPY_DBL_BYTES(dst, src) \
    do { \
        size_t s = 0; \
        for (; s < sizeof(double); ++s) { \
            (dst)[s] = (src)[sizeof(double) - 1 - s]; \
        } \
    } while(0)
#else
#   error    Unknown byte order!
#endif


SCISQL_LOCAL int scisql_s2cpoly_frombin(scisql_s2cpoly *out,
                                        const unsigned char *s,
                                        size_t len)
{
    _scisql_double_bytes dbytes;
    size_t i, n;
    if (out == 0 || s == 0) {
        return 1;
    }
    n = len / (3 * sizeof(double));
    if (n < 3 || n > SCISQL_MAX_VERTS || n * 3 * sizeof(double) != len) {
        return 1;
    }
    out->n = n;
    for (i = 0; i < n; ++i) {
        SCISQL_COPY_DBL_BYTES(dbytes.bytes, s);
        out->edges[i].x = dbytes.value;
        s += sizeof(double);
        SCISQL_COPY_DBL_BYTES(dbytes.bytes, s);
        out->edges[i].y = dbytes.value;
        s += sizeof(double);
        SCISQL_COPY_DBL_BYTES(dbytes.bytes, s);
        out->edges[i].z = dbytes.value;
    }
    return 0;
}

SCISQL_LOCAL size_t scisql_s2cpoly_tobin(unsigned char *out,
                                         size_t len,
                                         const scisql_s2cpoly *cp)
{
    _scisql_double_bytes dbytes;
    size_t i, n;
    if (out == 0 || cp == 0) {
        return 0;
    }
    n = cp->n;
    if (n * 3 * sizeof(double) < len) {
        return 0;
    }
    for (i = 0; i < n; ++i) {
        dbytes.value = cp->edges[i].x;
        SCISQL_COPY_DBL_BYTES(out, dbytes.bytes);
        out += sizeof(double);
        dbytes.value = cp->edges[i].y;
        SCISQL_COPY_DBL_BYTES(out, dbytes.bytes);
        out += sizeof(double);
        dbytes.value = cp->edges[i].z;
        SCISQL_COPY_DBL_BYTES(out, dbytes.bytes);
        out += sizeof(double);
    }
    return n * 3 * sizeof(double);
}

#undef SCISQL_COPY_DBL_BYTES


#ifdef __cplusplus
}
#endif


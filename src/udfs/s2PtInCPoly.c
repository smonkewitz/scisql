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
    ================================================================


    s2PtInCPoly(DOUBLE PRECISION lon,
                DOUBLE PRECISION lat,
                VARBINARY polyString)

    s2PtInCPoly(DOUBLE PRECISION lon,
                DOUBLE PRECISION lat,
                DOUBLE PRECISION v1Lon,
                DOUBLE PRECISION v1Lat,
                DOUBLE PRECISION v2Lon,
                DOUBLE PRECISION v2Lat,
                ...
                DOUBLE PRECISION vNLon,
                DOUBLE PRECISION vNLat)


    A MySQL UDF returning 1 if the point (lon, lat) lies inside the
    the given spherical convex polygon, and 0 otherwise. The polygon may
    be specified either as a VARBINARY byte string (as produced by
    s2CPolyToBin()), or as a sequence of at least 3 and at most 16
    vertex pairs.
    
    Example:
    --------

    SELECT scienceCcdExposureId
        FROM Science_Ccd_Exposure
        WHERE s2PtInCPoly(0.0, 0.0, ccdBoundary) = 1;

    Inputs:
    -------

    All arguments except polyString must be convertible to type
    DOUBLE PRECISION and are assumed to be in units of degrees. Note that:

    - If any parameter is NULL, 0 is returned.

    - If any coordinate is NaN or +/-Inf, this is an error and
      NULL is returned (IEEE specials are not currently supported by MySQL).

    - If any latitude angle lies outside of [-90, 90] degrees,
      this is an error and NULL is returned.

    - Polygon vertices can be specified in either clockwise or
      counter-clockwise order. However, vertices are assumed to be
      hemispherical, to define edges that do not intersect except at
      vertices, and to define edges that form a convex polygon.

    - As previously mentioned, input coordinates are coerced to be of type
      DOUBLE PRECISION. If the inputs are of type BIGINT or DECIMAL, then the
      coercion can result in loss of precision and hence an inaccurate result.
      Loss of precision will not occur so long as the inputs are values
      of type DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT, or TINYINT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mysql.h"

#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    int valid;
    int const_pos;
    int const_poly;
    scisql_v3 pos;
    scisql_s2cpoly poly;
} _scisql_ptpoly_state;


SCISQL_API my_bool s2PtInCPoly_init(UDF_INIT *initid,
                                    UDF_ARGS *args,
                                    char *message)
{
    size_t i;
    my_bool const_item = 1, const_pos = 1, const_poly = 1;

    if (args->arg_count != 3) {
        if (args->arg_count < 8 ||
            args->arg_count > 2 + 2 * SCISQL_MAX_VERTS ||
            (args->arg_count & 1) != 0) {
            snprintf(message, MYSQL_ERRMSG_SIZE, "s2PtInCPoly() expects "
                     "between 4 and %d spherical coordinate pairs",
                     SCISQL_MAX_VERTS + 1);
            return 1;
        }
    } else if (args->arg_type[2] != STRING_RESULT) {
        strncpy(message, "s2PtInCPoly() expects a spherical coordinate pair "
                "and a polygon byte string", MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
    }
    for (i = 0; i < 2; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] == 0) {
            const_item = 0;
            const_pos = 0;
        }
    }
    for (i = 2; i < args->arg_count; ++i) {
        if (args->arg_count != 3) {
            args->arg_type[i] = REAL_RESULT;
        }
        if (args->args[i] == 0) {
            const_item = 0;
            const_poly = 0;
        }
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    initid->ptr = 0;
    if (const_pos != 0 || const_poly != 0) {
        _scisql_ptpoly_state *state =
            (_scisql_ptpoly_state *) malloc(sizeof(_scisql_ptpoly_state));
        if (state != 0) {
            state->valid = 0;
            state->const_pos = const_pos;
            state->const_poly = const_poly;
            initid->ptr = (char *) state;
        }
    }
    return 0;
}


SCISQL_API long long s2PtInCPoly(UDF_INIT *initid,
                                 UDF_ARGS *args,
                                 char *is_null,
                                 char *error SCISQL_UNUSED)
{
    _scisql_ptpoly_state s;
    _scisql_ptpoly_state *state;
    size_t i, n;
    scisql_sc pt;
    scisql_v3 verts[SCISQL_MAX_VERTS];

    s.valid = 0;
    s.const_pos = 0;
    s.const_poly = 0;
    state = (initid->ptr == 0) ? &s : (_scisql_ptpoly_state *) initid->ptr;

    if (state->valid == 0 || state->const_pos == 0) {
        /* if position isn't constant or isn't cached yet, extract it
           from the arguments and convert it to a unit vector. */
        double **a = (double **) args->args;
        if (a[0] == 0 || a[1] == 0) {
            return 0;
        }
        if (scisql_sc_init(&pt, *a[0], *a[1]) != 0) {
            *is_null = 1;
            return 0;
        }
        scisql_sctov3(&state->pos, &pt);
    }
    if (state->valid == 0 || state->const_poly == 0) {
        /* if polygon isn't constant or isn't cached yet, build one
           from the arguments. */
        for (i = 2; i < args->arg_count; ++i) {
            if (args->args[i] == 0) {
                return 0;
            }
        }
        if (args->arg_count == 3) {
            i = scisql_s2cpoly_frombin(&state->poly,
                                       (unsigned char *) args->args[2],
                                       (size_t) args->lengths[2]);
            if (i != 0) {
                *is_null = 1;
                return 0;
            }
        } else {
            double **a = (double **) args->args;
            for (i = 2, n = 0; i < args->arg_count; i += 2, ++n) {
                if (scisql_sc_init(&pt, *a[i], *a[i + 1]) != 0) {
                    *is_null = 1;
                    return 0;
                }
                scisql_sctov3(&verts[n], &pt);
            }
            if (scisql_s2cpoly_init(&state->poly, verts, n) != 0) {
                *is_null = 1;
                return 0;
            }
        }
    }
    state->valid = 1;
    return scisql_s2cpoly_cv3(&state->poly, &state->pos);
}


SCISQL_API void s2PtInCPoly_deinit(UDF_INIT *initid) {
    free(initid->ptr);
}


#ifdef __cplusplus
} /* extern "C" */
#endif


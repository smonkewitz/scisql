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


    s2CPolyToBin(DOUBLE PRECISION v1Lon,
                 DOUBLE PRECISION v1Lat,
                 DOUBLE PRECISION v2Lon,
                 DOUBLE PRECISION v2Lat,
                 ...
                 DOUBLE PRECISION vNLon,
                 DOUBLE PRECISION vNLat)


    A MySQL UDF returning a byte-string representation of the given spherical
    convex polygon, and 0 otherwise. The polygon must be specified as a
    sequence of at least 3 and at most 20 vertex pairs. An N vertex input
    will result in a binary string of length exactly 12*N.

    Example:
    --------

    ALTER TABLE Science_Ccd_Exposure
        ADD COLUMN ccdBoundary BINARY(96);

    UPDATE Science_Ccd_Exposure
        SET ccdBoundary = s2CPolyToBin(llcRa, llcDecl,
                                       ulcRa, ulcDecl,
                                       urcRa, urcDecl,
                                       lrcRa, lrcDecl);

    Inputs:
    -------

    All arguments must be convertible to type DOUBLE PRECISION and are
    assumed to be in units of degrees. Note that:

    - If any parameter is NULL, 0 is returned.

    - If any coordinate is NaN or +/-Inf, this is an error and
      NULL is returned (IEEE specials are not currently supported by MySQL).

    - If any latitude angle lies outside of [-90, 90] degrees,
      this is an error and NULL is returned.

    - Polygon vertices can be specified in either clockwise or
      counter-clockwise order. However, the vertices are assumed to be
      hemispherical, to define edges that do not intersect except at
      vertices, and to define edges that form a convex polygon.

    - As previously mentioned, input coordinates are coerced to be of type
      DOUBLE PRECISION. If the inputs are of type BIGINT or DECIMAL, then the
      coercion can result in loss of precision and hence an inaccurate result.
      Loss of precision will not occur so long as the inputs are values
      of type DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT, or TINYINT.
 */
#include <stdio.h>

#include "mysql/mysql.h"

#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool s2CPolyToBin_init(UDF_INIT *initid,
                                     UDF_ARGS *args,
                                     char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count < 6 ||
        args->arg_count > 2 * SCISQL_MAX_VERTS ||
        (args->arg_count & 1) != 0) {
        snprintf(message, MYSQL_ERRMSG_SIZE, "s2CPolyToBin() expects between "
                 "3 and %d spherical coordinate pairs", SCISQL_MAX_VERTS);
        return 1;
    }
    for (i = 0; i < args->arg_count; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] == 0) {
            const_item = 0;
        }
    }
    initid->maybe_null = 1;
    initid->max_length = 3 * sizeof(double) * SCISQL_MAX_VERTS;
    initid->const_item = const_item;
    return 0;
}


SCISQL_API char * s2CPolyToBin(UDF_INIT *initid SCISQL_UNUSED,
                               UDF_ARGS *args,
                               char *result,
                               unsigned long *length,
                               char *is_null,
                               char *error SCISQL_UNUSED)
{
    scisql_s2cpoly poly;
    scisql_sc pt;
    scisql_v3 verts[SCISQL_MAX_VERTS];
    size_t i, n;
    double **a = (double **) args->args;

    /* If any input is NULL, the result is NULL. */
    for (i = 0; i < args->arg_count; ++i) {
        if (a[i] == 0) {
            *is_null = 1;
            return result;
        }
    }
    for (i = 0, n = 0; i < args->arg_count; i += 2, ++n) {
        if (scisql_sc_init(&pt, *a[i], *a[i + 1]) != 0) {
            *is_null = 1;
            return result;
        }
        scisql_sctov3(&verts[n], &pt);
    }
    if (scisql_s2cpoly_init(&poly, verts, n) != 0) {
        *is_null = 1;
        return result;
    }
    
    *length = (unsigned long) scisql_s2cpoly_tobin((unsigned char *) result,
                                                   255u, &poly);
    if (*length == 0) {
        *is_null = 1;
    }
    return result;
}


#ifdef __cplusplus
} /* extern "C" */
#endif


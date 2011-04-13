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


    s2PtInBox(DOUBLE PRECISION lon,
              DOUBLE PRECISION lat,
              DOUBLE PRECISION centerLon,
              DOUBLE PRECISION centerLat,
              DOUBLE PRECISION radius)

    A MySQL UDF returning 1 if the point (lon, lat) lies inside the
    the given spherical circle, and 0 otherwise.

    Example:
    --------

    SELECT objectId, ra_PS, decl_PS
        FROM Object
        WHERE s2PtInCircle(ra_PS, decl_PS, 0.0, 0.0, 1.0) = 1;

    Inputs:
    -------

    All arguments must be convertible to type DOUBLE PRECISION, and are
    assumed to be in units of degrees. Note that:

    - If any parameter is NULL, 0 is returned.

    - If any parameter is NaN or +/-Inf, this is an error and
      NULL is returned (IEEE specials are not currently supported by MySQL).

    - If lat or centerLat lie outside of [-90, 90] degrees,
      this is an error and NULL is returned.

    - If radius is negative or greater than 180, this is
      an error and NULL is returned.

    - As previously mentioned, input values are coerced to be of type
      DOUBLE PRECISION. If the inputs are of type BIGINT or DECIMAL, then the
      coercion can result in loss of precision and hence an inaccurate result.
      Loss of precision will not occur so long as the inputs are values
      of type DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT, or TINYINT.
 */
#include <stdlib.h>
#include <string.h>

#include "mysql.h"

#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    double dist2;
    int valid;
} _scisql_dist2_cache;


SCISQL_API my_bool s2PtInCircle_init(UDF_INIT *initid,
                                     UDF_ARGS *args,
                                     char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count != 5) {
        strncpy(message, "s2PtInCircle() expects exactly 5 arguments",
                MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    for (i = 0; i < 5; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] == 0) {
            const_item = 0;
        }
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    initid->ptr = 0;
    /* For constant radius circles, cache the square secant distance
       corresponding to the radius across calls. */
    if (args->args[4] != 0) {
        initid->ptr = (char *) calloc(1, sizeof(_scisql_dist2_cache));
    }
    return 0;
}


SCISQL_API long long s2PtInCircle(UDF_INIT *initid,
                                  UDF_ARGS *args,
                                  char *is_null,
                                  char *error SCISQL_UNUSED)
{
    scisql_sc p, cen;
    double r, angle;
    double **a = (double **) args->args;
    size_t i;
    /* If any input is null, the result is 0. */
    for (i = 0; i < 5; ++i) {
        if (a[i] == 0) {
            return 0;
        }
    }
    if (scisql_sc_init(&p, *a[0], *a[1]) != 0 ||
        scisql_sc_init(&cen, *a[2], *a[3]) != 0) {
        *is_null = 1;
        return 0;
    }
    r = *a[4];
    if (r < 0.0 || r > 180.0 || SCISQL_ISNAN(r)) {
        *is_null = 1;
        return 0;
    }
    /* Fail-fast if latitude angle delta exceeds the radius. */
    if (fabs(p.lat - cen.lat) > r) {
        return 0;
    }
    if (initid->ptr == 0) {
        angle = scisql_sc_angsep(&p, &cen);
    } else {
        _scisql_dist2_cache *cache = (_scisql_dist2_cache *) initid->ptr;
        if (cache->valid == 0) {
            /* Compute square secant distance corresponding to circle.
               Avoids an asin() and sqrt() for constant radii. */
            double d;
            d = sin(r * 0.5 * SCISQL_RAD_PER_DEG);
            cache->dist2 = 4.0 * d * d;
            cache->valid = 1;
        }
        r = cache->dist2;
        angle = scisql_sc_dist2(&p, &cen);
    }
    return angle <= r;
}


SCISQL_API void s2PtInCircle_deinit(UDF_INIT *initid) {
    free(initid->ptr);
}


#ifdef __cplusplus
} /* extern "C" */
#endif


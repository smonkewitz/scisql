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
              DOUBLE PRECISION lonMin,
              DOUBLE PRECISION latMin,
              DOUBLE PRECISION lonMax,
              DOUBLE PRECISION latMax)

    A MySQL UDF returning 1 if the point (lon, lat) lies inside the
    the given longitude/latitude angle box, and 0 otherwise.
    
    Example:
    --------

    SELECT objectId, ra_PS, decl_PS
        FROM Object
        WHERE s2PtInBox(ra_PS, decl_PS, -10.0, 10.0, 10.0, 20.0) = 1;

    Inputs:
    -------

    All arguments must be convertible to type DOUBLE PRECISION, and are
    assumed to be in units of degrees. Note that:

    - If any parameter is NULL, 0 is returned.

    - If any parameter is NaN or +/-Inf, this is an error and
      NULL is returned (IEEE specials are not currently supported by MySQL).

    - If lat, latMin or latMax lies outside of [-90, 90] degrees,
      this is an error and NULL is returned.

    - If latMin is greater than latMax, the box is considered to be empty
      and 0 is returned.

    - If both lonMin and lonMax lie in the range [0, 360], then lonMax
      may be less than lonMin. For example, a box with lonMin = 350
      and lonMax = 10 includes points with longitudes in the ranges
      [350, 360) and [0, 10].

    - If either lonMin or lonMax lies outside of [0, 360], then lonMin
      must be less than or equal to lonMax. Otherwise, NULL is returned.
      The values can however be arbitrarily large. If the two are separated
      by 360 degrees or more, then the box spans [0, 360) in longitude.
      Otherwise, lonMin and lonMax are range reduced. So for example, a
      spherical box with lonMin = 350 and lonMax = 370 includes longitudes
      in the ranges [350, 360) and [0, 10].

    - As previously mentioned, input values are coerced to be of type
      DOUBLE PRECISION. If the inputs are of type BIGINT or DECIMAL, then the
      coercion can result in loss of precision and hence an inaccurate result.
      Loss of precision will not occur so long as the inputs are values
      of type DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT, or TINYINT.
 */
#include <string.h>

#include "mysql/mysql.h"

#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool s2PtInBox_init(UDF_INIT *initid,
                                  UDF_ARGS *args,
                                  char *message)
{
    int i;
    my_bool const_item = 1;
    if (args->arg_count != 6) {
        strncpy(message, "ptInS2Box() expects 6 arguments",
                MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    for (i = 0; i < 6; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] == 0) {
            const_item = 0;
        }
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    return 0;
}


SCISQL_API long long s2PtInBox(UDF_INIT *initid SCISQL_UNUSED,
                               UDF_ARGS *args,
                               char *is_null,
                               char *error SCISQL_UNUSED)
{
    scisql_sc p, bmin, bmax;
    double **a = (double **) args->args;
    int i;

    /* If any input is null, the result is 0. */
    for (i = 0; i < 6; ++i) {
        if (a[i] == 0) {
            return 0;
        }
    }
    if (scisql_sc_init(&p, *a[0], *a[1]) != 0 ||
        scisql_sc_init(&bmin, *a[2], *a[3]) != 0 ||
        scisql_sc_init(&bmax, *a[4], *a[5]) != 0) {
        *is_null = 1;
        return 0;
    }
    
    if (bmax.lon < bmin.lon && (bmax.lon < 0.0 || bmin.lon > 360.0)) {
        *is_null = 1;
        return 0;
    }
    /* Check if latitude is in range */
    if (bmin.lat > bmax.lat || p.lat < bmin.lat || p.lat > bmax.lat) {
        return 0;
    }
    if (bmax.lon - bmin.lon >= 360.0) {
        return 1;
    }
    /* Range-reduce all longitude angles */
    p.lon = scisql_angred(p.lon);
    bmin.lon = scisql_angred(bmin.lon);
    bmax.lon = scisql_angred(bmax.lon);
    if (bmin.lon <= bmax.lon) {
        return p.lon >= bmin.lon && p.lon <= bmax.lon;
    } else {
        return p.lon >= bmin.lon || p.lon <= bmax.lon;
    }
}


#ifdef __cplusplus
} /* extern "C" */
#endif

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


    s2CircleHtmRanges(DOUBLE PRECISION centerLon,
                      DOUBLE PRECISION centerLat,
                      DOUBLE PRECISION radius,
                      INTEGER level,
                      INTEGER maxranges)

    A MySQL UDF returning a byte-string representation of HTM ID ranges
    overlapping a spherical circle.

    Inputs:
    -------

    The centerLon, centerLat, and radius arguments must be convertible
    to type DOUBLE PRECISION, and are assumed to be in units of degrees.
    The maxranges and level arguments must be integers. Note that:

    - If any parameter is NULL, 0 is returned.

    - If any parameter is NaN or +/-Inf, this is an error and
      NULL is returned (IEEE specials are not currently supported by MySQL).

    - If centerLat is not in the [-90, 90] degree range,
      this is an error and NULL is returned.

    - If radius is negative or greater than 180, this is
      an error and NULL is returned.

    - As previously mentioned, coordinates and radius are coerced to be of type
      DOUBLE PRECISION. If the inputs are of type BIGINT or DECIMAL, then the
      coercion can result in loss of precision and hence an inaccurate result.
      Loss of precision will not occur so long as the inputs are values
      of type DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT, or TINYINT.

    - level must be in the range [0, 24]

    - maxranges can be set to any value. In practice, its value is clamped
      such that the binary return string has size at most 16MB. Negative
      values are interpreted to mean "return as many ranges as possible,
      subject to the 16MB output size limit".
 */
#include <stdlib.h>
#include <stdio.h>

#include "mysql.h"

#include "htm.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool s2CircleHtmRanges_init(UDF_INIT *initid,
                                          UDF_ARGS *args,
                                          char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count != 5) {
        snprintf(message, MYSQL_ERRMSG_SIZE, "s2CircleHtmRanges() expects "
                 "exactly 5 arguments");
        return 1;
    }
    for (i = 0; i < 5; ++i) {
        if (i < 3) {
            args->arg_type[i] = REAL_RESULT;
        } else if (args->arg_type[i] != INT_RESULT) {
            snprintf(message, MYSQL_ERRMSG_SIZE, "s2CircleHtmRanges(): "
                     "fourth and fifth arguments must be integers");
            return 1;
        }
        if (args->args[i] == 0) {
            const_item = 0;
        }
    }
    initid->maybe_null = 1;
    initid->max_length = SCISQL_HTM_MAX_BLOB_SIZE;
    initid->const_item = const_item;
    initid->ptr = 0;
    return 0;
}


SCISQL_API char * s2CircleHtmRanges(UDF_INIT *initid,
                                    UDF_ARGS *args,
                                    char *result,
                                    unsigned long *length,
                                    char *is_null,
                                    char *error SCISQL_UNUSED)
{
    scisql_sc cen;
    scisql_v3 v;
    scisql_ids *ids;
    long long level;
    long long maxranges;
    double **a = (double **) args->args;
    double r;
    size_t i;

    /* If any input is NULL, the result is NULL. */
    for (i = 0; i < 5; ++i) {
        if (args->args[i] == 0) {
            *is_null = 1;
            return result;
        }
    }
    /* extract circle and subdivision parameters */
    if (scisql_sc_init(&cen, *a[0], *a[1]) != 0) {
        *is_null = 1;
        return result;
    }
    r = *a[2];
    if (r < 0.0 || r > 180.0 || SCISQL_ISNAN(r)) {
        *is_null = 1;
        return result;
    }
    level = *((long long *) args->args[3]);
    if (level < 0 || level > SCISQL_HTM_MAX_LEVEL) {
        *is_null = 1;
        return result;
    }
    maxranges = *((long long *) args->args[4]);
    if (maxranges < 0 || maxranges > (long long) SCISQL_HTM_MAX_RANGES) {
        maxranges = SCISQL_HTM_MAX_RANGES;
    }
    scisql_sctov3(&v, &cen);
    /* compute overlapping HTM ID ranges */
    ids = scisql_s2circle_htmids(
        (scisql_ids *) initid->ptr, &v, r, (int) level, (size_t) maxranges);
    initid->ptr = (char *)  ids;
    if (ids == 0) {
        *is_null = 1;
        return result;
    }
    *length = (unsigned long) (2 * sizeof(int64_t) * ids->n);
    return (char *) ids->ranges;
}


SCISQL_API void s2CircleHtmRanges_deinit(UDF_INIT *initid) {
    free(initid->ptr);
}


#ifdef __cplusplus
} /* extern "C" */
#endif


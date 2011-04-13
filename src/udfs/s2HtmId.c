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


    s2HtmId(DOUBLE PRECISION lon,
            DOUBLE PRECISION lat,
            INTEGER level)

    A MySQL UDF returning the HTM ID of the point (lon, lat) at the
    given subdivision level. The values returned are of type BIGINT.

    Example:
    --------

    SELECT objectId, ra_PS, decl_PS, s2HtmId(ra_PS, decl_PS)
        FROM Object;

    Inputs:
    -------

    The lon and lat arguments must be convertible to type DOUBLE PRECISION,
    and are assumed to be in units of degrees. The level argument must be
    convertible to an integer. Note that:

    - If any parameter is NULL, NULL is returned.

    - If lon or lat is NaN or +/-Inf, this is an error and
      NULL is returned (IEEE specials are not currently supported by MySQL).

    - If lat lies outside of [-90, 90] degrees, this is an error
      and NULL is returned.

    - If level is negative or greater than 24, this is
      an error and NULL is returned.

    - As previously mentioned, input coordinates are coerced to be of type
      DOUBLE PRECISION. If the inputs are of type BIGINT or DECIMAL, then the
      coercion can result in loss of precision and hence an inaccurate result.
      Loss of precision will not occur so long as the inputs are values
      of type DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT, or TINYINT.
 */
#include <stdlib.h>
#include <string.h>

#include "mysql.h"

#include "htm.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool s2HtmId_init(UDF_INIT *initid,
                                UDF_ARGS *args,
                                char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count != 3) {
        strncpy(message, "s2HtmId() expects exactly 3 arguments",
                MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    for (i = 0; i < 3; ++i) {
        args->arg_type[i] = (i < 2) ? REAL_RESULT : INT_RESULT;
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
        initid->ptr = (char *) malloc(sizeof(long long));
        *(long long *) initid->ptr = 0;
    }
    return 0;
}


SCISQL_API long long s2HtmId(UDF_INIT *initid,
                             UDF_ARGS *args,
                             char *is_null,
                             char *error SCISQL_UNUSED)
{
    scisql_sc p;
    scisql_v3 v;
    long long level, id;
    size_t i;
    /* If any input is null, the result is NULL. */
    for (i = 0; i < 3; ++i) {
        if (args->args[i] == 0) {
            *is_null = 1;
            return 0;
        }
    }
    if (initid->ptr != 0) {
        id = *(long long*) initid->ptr;
        if (id != 0) {
            return id;
        }
    }
    if (scisql_sc_init(&p, *(double *) args->args[0], *(double *) args->args[1]) != 0) {
        *is_null = 1;
        return 0;
    }
    level = *(long long *) args->args[2];
    if (level < 0 || level > SCISQL_HTM_MAX_LEVEL) {
        *is_null = 1;
        return 0;
    }
    scisql_sctov3(&v, &p);
    id = scisql_v3_htmid(&v, (int) level);
    if (id <= 0) {
        *is_null = 1;
        return 0;
    }
    if (initid->ptr != 0) {
        *(long long*) initid->ptr = id;
    }
    return id;
}


SCISQL_API void s2HtmId_deinit(UDF_INIT *initid) {
    free(initid->ptr);
}


#ifdef __cplusplus
} /* extern "C" */
#endif


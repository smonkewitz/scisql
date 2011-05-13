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
            DOUBLE PRECISION lat)

    s2HtmId(DOUBLE PRECISION lon,
            DOUBLE PRECISION lat,
            INTEGER level)

    A MySQL UDF returning the HTM ID of the point (lon, lat) at the
    given subdivision level. If no subdivision level is specified, a
    default of 20 is used (~0.3 arcsecond bins). The values returned
    are of type BIGINT.

    Example:
    --------

    SELECT objectId, ra_PS, decl_PS, s2HtmId(ra_PS, decl_PS)
        FROM Object;

    Inputs:
    -------

    The lon and lat arguments must be convertible to type DOUBLE PRECISION,
    and are assumed to be in units of degrees. The level argument must be
    an integer. Note that:

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
#include <stdio.h>

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
    if (args->arg_count < 2 || args->arg_count > 3) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "s2HtmId() expects 2 or 3 arguments");
        return 1;
    }
    for (i = 0; i < args->arg_count; ++i) {
        if (i < 2) {
            args->arg_type[i] = REAL_RESULT;
        } else if (args->arg_type[i] != INT_RESULT) {
            snprintf(message, MYSQL_ERRMSG_SIZE, "s2HtmId() subdivision "
                     "level must be an integer");
            return 1;
        }
        if (args->args[i] == 0) {
            const_item = 0;
        }
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    return 0;
}


SCISQL_API long long s2HtmId(UDF_INIT *initid SCISQL_UNUSED,
                             UDF_ARGS *args,
                             char *is_null,
                             char *error SCISQL_UNUSED)
{
    scisql_sc p;
    scisql_v3 v;
    long long level = SCISQL_HTM_DEF_LEVEL;
    long long id;
    size_t i;
    /* If any input is null, the result is NULL. */
    for (i = 0; i < args->arg_count; ++i) {
        if (args->args[i] == 0) {
            *is_null = 1;
            return 0;
        }
    }
    if (scisql_sc_init(&p, *(double *) args->args[0], *(double *) args->args[1]) != 0) {
        *is_null = 1;
        return 0;
    }
    if (args->arg_count > 2) {
        level = *(long long *) args->args[2];
    }
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
    return id;
}


#ifdef __cplusplus
} /* extern "C" */
#endif


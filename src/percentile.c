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


    percentile(DOUBLE PRECISION value, DOUBLE PRECISION percent)

    percentile() is a MySQL aggregate UDF. Given a set of N DOUBLE PRECISION
    values, it returns the value V such that at most floor(N * percent/100.0)
    of the values are less than V and at most N - floor(N * percent/100.0) are
    greater.

    The percent argument must not vary across the elements of a GROUP for
    which a percentile is computed, or the return value is undefined.

    Example:
    --------

    SELECT objectId,
           percentile(psfFlux, 25) AS firstQuartile,
           percentile(psfFlux, 75) AS thirdQuartile
        FROM Source
        WHERE objectId IS NOT NULL
        GROUP BY objectId;

    Inputs:
    -------

    A sequence of values convertible to type DOUBLE PRECISION, and a
    percentage (constant across the sequence), also convertible to type
    DOUBLE PRECISION. Note that:

    - NULL and NaN sequence values are ignored. MySQL does not currently
      support storage of NaNs.  However, their presence is checked for to
      ensure reasonable behaviour if a future MySQL release does end up
      supporting them.

    - If all sequence values are NULL or NaN, then NULL is returned.

    - If the percent argument is NULL or does not lie in the range [0, 100],
      NULL is returned.

    - If a sequence contains no values, NULL is returned.

    - If a sequence contains exactly one value, that is the value returned.

    - If (N - 1) * percent/100.0 = K is an integer, the value returned is the
      K-th smallest element in a sorted copy of the input sequence A.
      Otherwise, the return value is A[k] + f*(A[k + 1] - A[k]), where
      k = floor(K) and f = K - k.

    - As previously mentioned, input values are coerced to be of type
      DOUBLE PRECISION. If the inputs are of type BIGINT or DECIMAL, then the
      coercion can result in loss of precision and hence an inaccurate result.
      Loss of precision will not occur so long as percentile() is called on
      values of type DOUBLE PRECISION, FLOAT, INTEGER, SMALLINT, or TINYINT.

    - The percentile() function can handle a maximum of 2**28 (268,435,456)
      values per GROUP.
 */
#include <math.h>
#include <string.h>

#include "mysql/mysql.h"

#include "select.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool percentile_init(UDF_INIT* initid,
                                   UDF_ARGS* args,
                                   char* message)
{
    scisql_percentile_state *state;
    if (args->arg_count != 2) {
        strncpy(message, "percentile() expects 2 arguments",
                MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    state = scisql_percentile_state_new();
    if (state == 0) {
        strncpy(message, "percentile() failed to allocate memory for "
                "internal state", MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    args->arg_type[1] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->decimals = 31;
    initid->ptr = (char *) state;
    return 0;
}


SCISQL_API void percentile_deinit(UDF_INIT *initid) {
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    scisql_percentile_state_free(state);
    initid->ptr = 0;
}


SCISQL_API void percentile_clear(UDF_INIT *initid,
                                 char *is_null SCISQL_UNUSED,
                                 char *error SCISQL_UNUSED)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    scisql_percentile_state_clear(state);
}


SCISQL_API void percentile_add(UDF_INIT *initid,
                               UDF_ARGS *args,
                               char *is_null,
                               char *error)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    if (*is_null == 1) {
        return;
    } else if (state->n == 0) {
        double p;
        if (args->args[1] == 0) {
            *is_null = 1;
            return;
        }
        p = *(double*) args->args[1];
        if (SCISQL_ISNAN(p) || p < 0.0 || p > 100.0) {
            *is_null = 1;
            return;
        }
        state->fraction = p / 100.0;
    }
    if (scisql_percentile_state_add(state, (double*) args->args[0]) != 0) {
        *error = 1;
    }
}


SCISQL_API void percentile_reset(UDF_INIT *initid,
                                 UDF_ARGS *args,
                                 char *is_null,
                                 char *error)
{
    percentile_clear(initid, is_null, error);
    percentile_add(initid, args, is_null, error);
}


SCISQL_API double percentile(UDF_INIT *initid,
                             UDF_ARGS *args SCISQL_UNUSED,
                             char *is_null,
                             char *error)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    if (state->n == 0 || *error != 0 || *is_null != 0) {
        *is_null = 1;
        return 0.0;
    }
    return scisql_percentile(state);
}


#ifdef __cplusplus
}
#endif

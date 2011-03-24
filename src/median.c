/*
    Copyright (C) 2011 Jacek Becla

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
        - Jacek Becla, SLAC
        - Serge Monkewitz, IPAC/Caltech

    Work on this project has been sponsored by LSST and SLAC/DOE.
    ================================================================


    median(DOUBLE PRECISION value)

    median() is a MySQL aggregate UDF returning the median of a sequence of
    DOUBLE PRECISION values.

    Example:
    --------

    SELECT objectId, median(psfFlux)
        FROM Source
        WHERE objectId IS NOT NULL
        GROUP BY objectId;

    Inputs:
    -------

    A set of values convertible to type DOUBLE PRECISION. Note that:

    - NULL and NaN values are ignored. MySQL does not currently support
      storage of NaNs.  However, their presence is checked for to ensure
      reasonable behaviour if a future MySQL release does end up supporting
      them.

    - If all inputs are NULL/NaN, then NULL is returned.

    - If there are no inputs, NULL is returned.

    - If there are an even number of elements in the input sequence, the
      return value is the mean of the two middle elements in a sorted copy
      of the sequence.

    - As previously mentioned, input values are coerced to be of type
      DOUBLE PRECISION. If the inputs are of type BIGINT or DECIMAL, then the
      coercion can result in loss of precision and hence an inaccurate result.
      Loss of precision will not occur so long as median() is called on
      values of type DOUBLE PRECISION, FLOAT, INTEGER, SMALLINT, or TINYINT.

    - The median() function can handle a maximum of 2**28 (268,435,456)
      values per GROUP.
 */
#include <string.h>

#include "mysql/mysql.h"

#include "select.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool median_init(UDF_INIT *initid,
                               UDF_ARGS *args,
                               char *message)
{
    scisql_percentile_state *state;
    if (args->arg_count != 1) {
        strncpy(message, "median() expects 1 argument", MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    state = scisql_percentile_state_new();
    if (state == 0) {
        strncpy(message, "median() failed to allocate memory for "
                "internal state", MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    initid->maybe_null = 1;
    initid->decimals = 31;
    initid->ptr = (char *) state;
    return 0;
}


SCISQL_API void median_deinit(UDF_INIT *initid) {
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    scisql_percentile_state_free(state);
    initid->ptr = 0;
}


SCISQL_API void median_clear(UDF_INIT *initid,
                             char *is_null SCISQL_UNUSED,
                             char *error SCISQL_UNUSED)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    scisql_percentile_state_clear(state);
}


SCISQL_API void median_add(UDF_INIT *initid,
                           UDF_ARGS *args,
                           char *is_null SCISQL_UNUSED,
                           char *error)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    if (scisql_percentile_state_add(state, (double*) args->args[0]) != 0) {
        *error = 1;
    }
}


SCISQL_API void median_reset(UDF_INIT *initid,
                             UDF_ARGS *args,
                             char *is_null,
                             char *error)
{
    median_clear(initid, is_null, error);
    median_add(initid, args, is_null, error);
}


SCISQL_API double median(UDF_INIT *initid,
                         UDF_ARGS *args SCISQL_UNUSED,
                         char *is_null,
                         char *error)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    if (state->n == 0 || *error != 0) {
        *is_null = 1;
        return 0.0;
    }
    return scisql_percentile(state);
}


#ifdef __cplusplus
}
#endif

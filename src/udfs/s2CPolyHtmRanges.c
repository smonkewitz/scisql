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
*/

/**
<udf name="${SCISQL_PREFIX}s2CPolyHtmRanges"
     return_type="MEDIUMBLOB"
     section="s2"
     internal="true">

    <desc>
        Returns a binary-string representation of HTM ID ranges
        overlapping a spherical convex polygon. The polygon must
        be specified in binary-string form (as produced by
        ${SCISQL_PREFIX}s2CPolyToBin()).
    </desc>
    <args>
        <arg name="poly" type="BINARY">
            Binary string representation of a polygon.
        </arg>
        <arg name="level" type="INTEGER">
            HTM subdivision level, must be in range [0, 24].
        </arg>
        <arg name="maxranges" type="INTEGER">
            Maximum number of ranges to report.
        </arg>
    </args>
    <notes>
        <note>
            If any parameter is NULL, this is an error
            and NULL is returned.
        </note>
        <note>
            If poly does not correspond to a valid binary serialization
            of a spherical convex polygon, this is an error and NULL
            is returned.
        </note>
        <note>
            If level does not lie in the range [0, 24], this is an
            error and NULL is returned.
        </note>
        <note>
            maxranges can be set to any value. In practice, its value
            is clamped such that the binary return string has size at
            most 16MB (fits in a MEDIUMBLOB). Negative values are
            interpreted to mean: "return as many ranges as possible
            subject to the 16MB output size limit".
        </note>
    </notes>
</udf>
*/

#include <stdlib.h>
#include <stdio.h>

#include "mysql.h"

#include "udf.h"
#include "htm.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(s2CPolyHtmRanges, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count != 3) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(s2CPolyHtmRanges)
                 " expects exactly 3 arguments");
        return 1;
    }
    if (args->arg_type[0] != STRING_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(s2CPolyHtmRanges)
                 ": first argument must be a binary string");
        return 1;
    }
    if (args->arg_type[1] != INT_RESULT || args->arg_type[2] != INT_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(s2CPolyHtmRanges)
                 ": second and third arguments must be integers");
        return 1;
    }
    for (i = 0; i < 3; ++i) {
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


SCISQL_API char * SCISQL_VERSIONED_FNAME(s2CPolyHtmRanges, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *result,
    unsigned long *length,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    scisql_s2cpoly poly;
    scisql_ids *ids;
    size_t i;
    long long level;
    long long maxranges;

    /* If any input is NULL, the result is NULL. */
    for (i = 0; i < 3; ++i) {
        if (args->args[i] == 0) {
            *is_null = 1;
            return result;
        }
    }
    /* extract polygon and subdivision parameters */
    i = scisql_s2cpoly_frombin(&poly, (unsigned char *) args->args[0],
                               (size_t) args->lengths[0]);
    if (i != 0) {
        *is_null = 1;
        return result;
    }
    level = *((long long *) args->args[1]);
    if (level < 0 || level > SCISQL_HTM_MAX_LEVEL) {
        *is_null = 1;
        return result;
    }
    maxranges = *((long long *) args->args[2]);
    if (maxranges < 0 || maxranges > (long long) SCISQL_HTM_MAX_RANGES) {
        maxranges = (long long) SCISQL_HTM_MAX_RANGES;
    }
    /* compute overlapping HTM ID ranges */
    ids = scisql_s2cpoly_htmids(
        (scisql_ids *) initid->ptr, &poly, (int) level, (size_t) maxranges);
    initid->ptr = (char *)  ids;
    if (ids == 0) {
        *is_null = 1;
        return result;
    }
    *length = (unsigned long) (2 * sizeof(int64_t) * ids->n);
    return (char *) ids->ranges;
}


SCISQL_API void SCISQL_VERSIONED_FNAME(s2CPolyHtmRanges, _deinit) (
    UDF_INIT *initid)
{
    free(initid->ptr);
}


SCISQL_UDF_INIT(s2CPolyHtmRanges)
SCISQL_UDF_DEINIT(s2CPolyHtmRanges)
SCISQL_STRING_UDF(s2CPolyHtmRanges)


#ifdef __cplusplus
} /* extern "C" */
#endif


/*
    Copyright (C) 2011 Serge Monkewitz

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    Authors:
        - Serge Monkewitz, IPAC/Caltech

    Work on this project has been sponsored by LSST and SLAC/DOE.
*/

/**
<udf name="${SCISQL_PREFIX}s2HtmId" return_type="BIGINT" section="s2">
    <desc>
        Returns the HTM ID of a point at the given
        subdivision level.
    </desc>
    <args>
        <arg name="lon" type="DOUBLE PRECISION" units="deg">
            Longitude angle of the point to index.
        </arg>
        <arg name="lat" type="DOUBLE PRECISION" units="deg">
            Latitude angle of the point to index.
        </arg>
        <arg name="level" type="INTEGER">
            HTM subdivision level, required to lie in the range [0, 24].
        </arg>
    </args>
    <notes>
        <note>
            If any parameter is NULL, NULL is returned.
        </note>
        <note>
            If lon or lat is NaN or +/-Inf, this is an error and NULL is
            returned (IEEE specials are not currently supported by MySQL).
        </note>
        <note>
            If lat lies outside of [-90, 90] degrees, this is an error
            and NULL is returned.
        </note>
        <note>
            If level is not in the range [0, 24], this is an error
            and NULL is returned.
        </note>
        <note>
            The lon and lat arguments must be convertible to type DOUBLE
            PRECISION. If their actual type is BIGINT or DECIMAL, then the
            conversion can result in loss of precision and hence an inaccurate
            result. Loss of precision will not occur so long as the inputs are
            values of type DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT or
            TINYINT.
        </note>
    </notes>
    <example>
        SELECT objectId, ra_PS, decl_PS, ${SCISQL_PREFIX}s2HtmId(ra_PS, decl_PS, 20)
            FROM Object LIMIT 10;
    </example>
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


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(s2HtmId, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count != 3) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(s2HtmId) " expects exactly 3 arguments");
        return 1;
    }
    for (i = 0; i < 3; ++i) {
        if (i < 2) {
            args->arg_type[i] = REAL_RESULT;
        } else if (args->arg_type[i] != INT_RESULT) {
            snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(s2HtmId)
                     " subdivision level must be an integer");
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


SCISQL_API long long SCISQL_VERSIONED_FNAME(s2HtmId, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    scisql_sc p;
    scisql_v3 v;
    long long level;
    long long id;
    size_t i;
    /* If any input is null, the result is NULL. */
    for (i = 0; i < 3; ++i) {
        if (args->args[i] == 0) {
            *is_null = 1;
            return 0;
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
    return id;
}


SCISQL_UDF_INIT(s2HtmId)
SCISQL_INTEGER_UDF(s2HtmId)


#ifdef __cplusplus
} /* extern "C" */
#endif


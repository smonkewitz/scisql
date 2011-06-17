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
<udf name="${SCISQL_PREFIX}median"
     return_type="DOUBLE PRECISION"
     section="statistics"
     aggregate="true">

    <desc>
        Returns the median of a GROUP of values.
    </desc>
    <args>
        <arg name="value" type="DOUBLE PRECISION">
            Value, column name or expression yielding input values.
        </arg>
    </args>
    <notes>
        <note>
            NULL and NaN values are ignored. MySQL does not currently support
            storage of NaNs.  However, their presence is checked for to ensure
            reasonable behaviour if a future MySQL release does end up
            supporting them.
        </note>
        <note>
            If all input values for a GROUP are NULL/NaN, then NULL is returned.
        </note>
        <note>
            If there are no inputs, NULL is returned.
        </note>
        <note>
            If there are an even number of elements in the input GROUP,
            the return value is the mean of the two middle elements in a
            sorted copy of the GROUP.
        </note>
        <note>
            As previously mentioned, input values are coerced to be of type
            DOUBLE PRECISION. If the inputs are of type BIGINT or DECIMAL,
            then the coercion can result in loss of precision and hence an
            inaccurate result. Loss of precision will not occur so long as
            median() is called on values of type DOUBLE PRECISION, FLOAT,
            INTEGER, SMALLINT, or TINYINT.
        </note>
        <note>
            This UDF can handle a maximum of 2<sup>27</sup> (134,217,728)
            input values per GROUP.
        </note>
    </notes>
    <example>
        SELECT objectId, ${SCISQL_PREFIX}median(psfFlux)
            FROM Source
            WHERE objectId IS NOT NULL
            GROUP BY objectId;
    </example>
</udf>
*/

#include <stdio.h>

#include "mysql.h"

#include "udf.h"
#include "select.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(median, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    scisql_percentile_state *state;
    if (args->arg_count != 1) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(median) " expects 1 argument");
        return 1;
    }
    state = scisql_percentile_state_new();
    if (state == 0) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(median)
                 " failed to allocate memory for internal state");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->decimals = 31;
    initid->ptr = (char *) state;
    return 0;
}


SCISQL_API void SCISQL_VERSIONED_FNAME(median, _deinit) (UDF_INIT *initid) {
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    scisql_percentile_state_free(state);
    initid->ptr = 0;
}


SCISQL_API void SCISQL_VERSIONED_FNAME(median, _clear) (
    UDF_INIT *initid,
    char *is_null SCISQL_UNUSED,
    char *error SCISQL_UNUSED)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    scisql_percentile_state_clear(state);
}


SCISQL_API void SCISQL_VERSIONED_FNAME(median, _add) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *is_null SCISQL_UNUSED,
    char *error)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    if (scisql_percentile_state_add(state, (double*) args->args[0]) != 0) {
        *error = 1;
    }
}


SCISQL_API void SCISQL_VERSIONED_FNAME(median, _reset) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *is_null,
    char *error)
{
    SCISQL_VERSIONED_FNAME(median, _clear) (initid, is_null, error);
    SCISQL_VERSIONED_FNAME(median, _add) (initid, args, is_null, error);
}


SCISQL_API double SCISQL_VERSIONED_FNAME(median, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid,
    UDF_ARGS *args SCISQL_UNUSED,
    char *is_null,
    char *error)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    if (state->n == 0 || *error != 0) {
        *is_null = 1;
        return 0.0;
    }
    return scisql_percentile_state_get(state);
}


SCISQL_UDF_INIT(median)
SCISQL_UDF_DEINIT(median)
SCISQL_UDF_CLEAR(median)
SCISQL_UDF_ADD(median)
SCISQL_UDF_RESET(median)
SCISQL_REAL_UDF(median)


#ifdef __cplusplus
}
#endif

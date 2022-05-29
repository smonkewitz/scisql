/*
    Copyright (C) 2011-2022 the SciSQL authors

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
<udf name="${SCISQL_PREFIX}percentile"
     return_type="DOUBLE PRECISION"
     section="statistics"
     aggregate="true">

    <desc>
        Returns the desired percentile of a GROUP of values.

        Given a GROUP of N DOUBLE PRECISION values, percentile returns the
        value V such that at most floor(N * percent/100.0) of the values are
        less than V and at most N - floor(N * percent/100.0) are greater.

        The percent argument must not vary across the elements of a GROUP for
        which a percentile is being computed, or the return value is undefined.
    </desc>
    <args>
        <arg name="value" type="DOUBLE PRECISION">
            Value, column name, or expression yielding input values.
        </arg>
        <arg name="percent" type="DOUBLE PRECISION">
            Desired percentile, must lie in the range [0, 100].
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
            If all inputs are NULL/NaN, then NULL is returned.
        </note>
        <note>
            If there are no input values, NULL is returned.
        </note>
        <note>
            If the input GROUP contains exactly one value, that value is
            returned.
        </note>
        <note>
            If the percent argument is NULL or does not lie in the range
            [0, 100], NULL is returned.
        </note>
        <note>
            If (N - 1) * percent/100.0 = K is an integer, the value returned
            is the K-th smallest element in a sorted copy of the input GROUP A.
            Otherwise, the return value is A[k] + f*(A[k + 1] - A[k]), where
            k = floor(K) and f = K - k.
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
        SELECT objectId,
               ${SCISQL_PREFIX}percentile(psfFlux, 25) AS firstQuartile,
               ${SCISQL_PREFIX}percentile(psfFlux, 75) AS thirdQuartile
            FROM Source
            WHERE objectId IS NOT NULL
            GROUP BY objectId
            LIMIT 10;
    </example>
</udf>
*/

#include <math.h>
#include <stdio.h>

#include "mysql.h"

#include "udf.h"
#include "select.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(percentile, _init) (
    UDF_INIT* initid,
    UDF_ARGS* args,
    char* message)
{
    scisql_percentile_state *state;
    if (args->arg_count != 2) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(percentile) " expects 2 arguments");
        return 1;
    }
    state = scisql_percentile_state_new();
    if (state == 0) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(percentile)
                 " failed to allocate memory for internal state");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    args->arg_type[1] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->decimals = 31;
    initid->ptr = (char *) state;
    return 0;
}


SCISQL_API void SCISQL_VERSIONED_FNAME(percentile, _deinit) (UDF_INIT *initid) {
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    scisql_percentile_state_free(state);
    initid->ptr = 0;
}


SCISQL_API void SCISQL_VERSIONED_FNAME(percentile, _clear) (
    UDF_INIT *initid,
    char *is_null SCISQL_UNUSED,
    char *error SCISQL_UNUSED)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    scisql_percentile_state_clear(state);
}


SCISQL_API void SCISQL_VERSIONED_FNAME(percentile, _add) (
    UDF_INIT *initid,
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


SCISQL_API void SCISQL_VERSIONED_FNAME(percentile, _reset) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *is_null,
    char *error)
{
    SCISQL_VERSIONED_FNAME(percentile, _clear) (initid, is_null, error);
    SCISQL_VERSIONED_FNAME(percentile, _add) (initid, args, is_null, error);
}


SCISQL_API double SCISQL_VERSIONED_FNAME(percentile, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid,
    UDF_ARGS *args SCISQL_UNUSED,
    char *is_null,
    char *error)
{
    scisql_percentile_state *state = (scisql_percentile_state *) initid->ptr;
    if (state->n == 0 || *error != 0 || *is_null != 0) {
        *is_null = 1;
        return 0.0;
    }
    return scisql_percentile_state_get(state);
}


SCISQL_UDF_INIT(percentile)
SCISQL_UDF_DEINIT(percentile)
SCISQL_UDF_CLEAR(percentile)
SCISQL_UDF_ADD(percentile)
SCISQL_UDF_RESET(percentile)
SCISQL_REAL_UDF(percentile)


#ifdef __cplusplus
}
#endif

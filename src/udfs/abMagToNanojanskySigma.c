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
        - Fritz Mueller, SLAC National Accelerator Laboratory

    Work on this project has been sponsored by LSST and SLAC/DOE.
*/

/**
<udf name="${SCISQL_PREFIX}abMagToNanojanskySigma"
     return_type="DOUBLE PRECISION"
     section="photometry">

    <desc>
        Converts an AB magnitude error to a calibrated flux error.
        The return value is in nanojanskys.
    </desc>
    <args>
        <arg name="mag" type="DOUBLE PRECISION" units="mag">
            AB magnitude.
        </arg>
        <arg name="magSigma" type="DOUBLE PRECISION" units="mag">
            Standard deviation of mag.
        </arg>
    </args>
    <notes>
        <note>
            All arguments must be convertible to type DOUBLE PRECISION.
        </note>
        <note>
            If any argument is NULL, NaN, or +/-Inf, NULL is returned.
        </note>
        <note>
            If magSigma is negative, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}abMagToNanojanskySigma(20.5, 0.01);
    </example>
</udf>
*/

#include <stdio.h>

#include "mysql.h"

#include "udf.h"
#include "photometry.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API SCISQL_BOOL SCISQL_VERSIONED_FNAME(abMagToNanojanskySigma, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 2) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(abMagToNanojanskySigma)
                 " expects exactly 2 arguments");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    args->arg_type[1] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->const_item = (args->args[0] != 0 && args->args[1] != 0);
    initid->decimals = 31;
    return 0;
}


SCISQL_API double SCISQL_VERSIONED_FNAME(abMagToNanojanskySigma, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    double **a = (double **) args->args;
    double fluxsigma;
    if (a[0] == 0 || a[1] == 0 ||
        SCISQL_ISSPECIAL(*a[0]) || SCISQL_ISSPECIAL(*a[1]) ||
        *a[1] < 0.0) {
        *is_null = 1;
        return 0.0;
    }
    fluxsigma = scisql_ab2nanojanskysigma(*a[0], *a[1]);
    if (SCISQL_ISSPECIAL(fluxsigma)) {
        *is_null = 1;
        return 0.0;
    }
    return fluxsigma;
}


SCISQL_UDF_INIT(abMagToNanojanskySigma)
SCISQL_REAL_UDF(abMagToNanojanskySigma)


#ifdef __cplusplus
} /* extern "C" */
#endif


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
<udf name="${SCISQL_PREFIX}abMagToNanojansky"
     return_type="DOUBLE PRECISION"
     section="photometry">

    <desc>
        Converts an AB magnitude to a calibrated flux.
        The return value is in nanojanskys.
    </desc>
    <args>
        <arg name="mag" type="DOUBLE PRECISION" units="mag">
            AB magnitude to convert to a calibrated flux.
        </arg>
    </args>
    <notes>
        <note>
            The mag argument must be convertible to type DOUBLE PRECISION.
        </note>
        <note>
            If the mag argument is NULL, NaN, or +/-Inf, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}abMagToNanojansky(20.5);
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


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(abMagToNanojansky, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 1) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(abMagToNanojansky) " expects exactly 1 argument");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->const_item = (args->args[0] != 0);
    initid->decimals = 31;
    return 0;
}


SCISQL_API double SCISQL_VERSIONED_FNAME(abMagToNanojansky, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    double **a = (double **) args->args;
    double flux;
    if (a[0] == 0 || SCISQL_ISSPECIAL(*a[0])) {
        *is_null = 1;
        return 0.0;
    }
    flux = scisql_ab2nanojansky(*a[0]);
    if (SCISQL_ISSPECIAL(flux)) {
        *is_null = 1;
        return 0.0;
    }
    return flux;
}


SCISQL_UDF_INIT(abMagToNanojansky)
SCISQL_REAL_UDF(abMagToNanojansky)


#ifdef __cplusplus
} /* extern "C" */
#endif


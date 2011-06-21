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
<udf name="${SCISQL_PREFIX}abMagToDnSigma"
     return_type="DOUBLE PRECISION"
     section="photometry">

    <desc>
        Converts an AB magnitude error to a raw flux error in DN.
    </desc>
    <args>
        <arg name="mag" type="DOUBLE PRECISION" units="mag">
            AB magnitude.
        </arg>
        <arg name="magSigma" type="DOUBLE PRECISION" units="mag">
            Standard deviation of mag.
        </arg>
        <arg name="fluxMag0" type="DOUBLE PRECISION" units="DN">
            Raw flux of a zero-magnitude object.
        </arg>
        <arg name="fluxMag0Sigma" type="DOUBLE PRECISION" units="DN">
            Standard deviation of fluxMag0.
        </arg>
    </args>
    <notes>
        <note>
            All arguments must be convertible to type DOUBLE PRECISION.
        </note>
        <note>
            If any argument is NULL, NaN, or +/-Inf, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}abMagToDnSigma(20.5, 0.01, 3.0e+12, 0.0);
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


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(abMagToDnSigma, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count != 4) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(abMagToDnSigma)
                 " expects exactly 4 arguments");
        return 1;
    }
    for (i = 0; i < 4; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] != 0) {
            const_item = 0;
        } 
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    initid->decimals = 31;
    return 0;
}


SCISQL_API double SCISQL_VERSIONED_FNAME(abMagToDnSigma, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    double **a = (double **) args->args;
    size_t i;
    for (i = 0; i < 4; ++i) {
        if (args->args[i] == 0) {
            *is_null = 1;
            return 0.0;
        }
    }
    return scisql_ab2dnsigma(*a[0], *a[1], *a[2], *a[3]);
}


SCISQL_UDF_INIT(abMagToDnSigma)
SCISQL_REAL_UDF(abMagToDnSigma)


#ifdef __cplusplus
} /* extern "C" */
#endif


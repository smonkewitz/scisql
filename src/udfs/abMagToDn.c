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
<udf name="${SCISQL_PREFIX}abMagToDn"
     return_type="DOUBLE PRECISION"
     section="photometry">

    <desc>
        Converts an AB magnitude to a raw flux in DN.
    </desc>
    <args>
        <arg name="mag" type="DOUBLE PRECISION" units="mag">
            AB magnitude to convert to a raw flux.
        </arg>
        <arg name="fluxMag0" type="DOUBLE PRECISION" units="DN">
            Raw flux of a zero-magnitude object.
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
            If fluxMag0 is negative or zero, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}abMagToDn(20.5, 3.0e+12);
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


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(abMagToDn, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 2) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(abMagToDn) " expects exactly 2 arguments");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    args->arg_type[1] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->const_item = (args->args[0] != 0 && args->args[1] != 0);
    initid->decimals = 31;
    return 0;
}


SCISQL_API double SCISQL_VERSIONED_FNAME(abMagToDn, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    double **a = (double **) args->args;
    double dn;
    if (a[0] == 0 || a[1] == 0 ||
        SCISQL_ISSPECIAL(*a[0]) || SCISQL_ISSPECIAL(*a[1]) ||
        *a[1] <= 0.0) {
        *is_null = 1;
        return 0.0;
    }
    dn = scisql_ab2dn(*a[0], *a[1]);
    if (SCISQL_ISSPECIAL(dn)) {
        *is_null = 1;
        return 0.0;
    }
    return dn;
}


SCISQL_UDF_INIT(abMagToDn)
SCISQL_REAL_UDF(abMagToDn)


#ifdef __cplusplus
} /* extern "C" */
#endif


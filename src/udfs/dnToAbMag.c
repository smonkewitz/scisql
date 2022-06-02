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
<udf name="${SCISQL_PREFIX}dnToAbMag"
     return_type="DOUBLE PRECISION"
     section="photometry">

    <desc>
        Converts a raw flux in DN to an AB magnitude.
    </desc>
    <args>
        <arg name="dn" type="DOUBLE PRECISION" units="DN">
            Raw flux to convert to an AB magnitude.
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
            If either fluxMag0 or dn is negative or zero, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}dnToAbMag(src.psfFlux, ccd.fluxMag0)
            FROM Source AS src, Science_Ccd_Exposure ccd
            WHERE src.scienceCcdExposureId = ccd.scienceCcdExposureId
            LIMIT 10;
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


SCISQL_API SCISQL_BOOL SCISQL_VERSIONED_FNAME(dnToAbMag, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 2) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(dnToAbMag) " expects exactly 2 arguments");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    args->arg_type[1] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->const_item = (args->args[0] != 0 && args->args[1] != 0);
    initid->decimals = 31;
    return 0;
}


SCISQL_API double SCISQL_VERSIONED_FNAME(dnToAbMag, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    double **a = (double **) args->args;
    double ab;

    if (a[0] == 0 || a[1] == 0 ||
        SCISQL_ISSPECIAL(*a[0]) || SCISQL_ISSPECIAL(*a[1]) ||
        *a[0] <= 0.0 || *a[1] <= 0.0) {
        *is_null = 1;
        return 0.0;
    }
    ab = scisql_dn2ab(*a[0], *a[1]);
    if (SCISQL_ISSPECIAL(ab)) {
        *is_null = 1;
        return 0.0;
    }
    return ab;
}


SCISQL_UDF_INIT(dnToAbMag)
SCISQL_REAL_UDF(dnToAbMag)


#ifdef __cplusplus
} /* extern "C" */
#endif


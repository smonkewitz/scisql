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
<udf name="${SCISQL_PREFIX}fluxToDn"
     return_type="DOUBLE PRECISION"
     section="photometry">

    <desc>
        Converts a calibrated (AB) flux to a raw DN value.
    </desc>
    <args>
        <arg name="flux" type="DOUBLE PRECISION"
             units="erg/cm&lt;sup&gt;2&lt;/sup&gt;/sec/Hz">
            Calibrated flux to convert to a raw DN value.
        </arg>
        <arg name="fluxMag0" type="DOUBLE PRECISION" units="DN">
            Raw flux of a zero-magnitude object.
        </arg>
    </args>
    <notes>
        <note>
            Both arguments must be convertible to type DOUBLE PRECISION.
        </note>
        <note>
            If either argument is NULL, NaN, or +/-Inf, NULL is returned.
        </note>
        <note>
            If fluxMag0 is negative or zero, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}fluxToDn(rFlux_PS, 3.0e+12)
            FROM Object
            WHERE rFlux_PS IS NOT NULL
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


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(fluxToDn, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 2) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(fluxToDn) " expects exactly 2 arguments");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    args->arg_type[1] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->const_item = (args->args[0] != 0 && args->args[1] != 0);
    initid->decimals = 31;
    return 0;
}


SCISQL_API double SCISQL_VERSIONED_FNAME(fluxToDn, SCISQL_NO_SUFFIX) (
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
    dn = scisql_flux2dn(*a[0], *a[1]);
    if (SCISQL_ISSPECIAL(dn)) {
        *is_null = 1;
        return 0.0;
    }
    return dn;
}


SCISQL_UDF_INIT(fluxToDn)
SCISQL_REAL_UDF(fluxToDn)


#ifdef __cplusplus
} /* extern "C" */
#endif


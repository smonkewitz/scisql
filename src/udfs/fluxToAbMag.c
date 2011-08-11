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
<udf name="${SCISQL_PREFIX}fluxToAbMag"
     return_type="DOUBLE PRECISION"
     section="photometry">

    <desc>
        Converts a calibrated (AB) flux to an AB magnitude.
    </desc>
    <args>
        <arg name="flux" type="DOUBLE PRECISION"
             units="erg/cm&lt;sup&gt;2&lt;/sup&gt;/sec/Hz">
            Calibrated flux to convert to an AB magnitude.
        </arg>
    </args>
    <notes>
        <note>
            The flux argument must be convertible to type DOUBLE PRECISION.
        </note>
        <note>
            If the flux argument is NULL, NaN, or +/-Inf, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}fluxToAbMag(rFlux_PS)
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


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(fluxToAbMag, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 1) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(fluxToAbMag) " expects exactly 1 argument");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->const_item = (args->args[0] != 0);
    initid->decimals = 31;
    return 0;
}


SCISQL_API double SCISQL_VERSIONED_FNAME(fluxToAbMag, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    if (args->args[0] == 0) {
        *is_null = 1;
        return 0.0;
    }
    return scisql_flux2ab(*((double *) args->args[0]));
}


SCISQL_UDF_INIT(fluxToAbMag)
SCISQL_REAL_UDF(fluxToAbMag)


#ifdef __cplusplus
} /* extern "C" */
#endif


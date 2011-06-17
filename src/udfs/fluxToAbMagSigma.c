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
<udf name="${SCISQL_PREFIX}fluxToAbMagSigma"
     return_type="DOUBLE PRECISION"
     section="photometry">

    <desc>
        Converts a cailbrated (AB) flux error to an AB magnitude error.
    </desc>
    <args>
        <arg name="flux" type="DOUBLE PRECISION"
             units="erg/cm&lt;sup&gt;2&lt;/sup&gt;/sec/Hz">
            Calibrated (AB) flux.
        </arg>
        <arg name="fluxSigma" type="DOUBLE PRECISION"
             units="erg/cm&lt;sup&gt;2&lt;/sup&gt;/sec/Hz">
            Calibrated (AB) flux error.
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
            If the flux argument is zero, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}fluxToAbMagSigma(rFlux_PS, rFlux_PS_Sigma)
            FROM Object
            WHERE rFlux_PS IS NOT NULL and rFlux_PS_Sigma IS NOT NULL
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


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(fluxToAbMagSigma, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 2) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(fluxToAbMagSigma)
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


SCISQL_API double SCISQL_VERSIONED_FNAME(fluxToAbMagSigma, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    if (args->args[0] == 0 || args->args[1] == 0) {
        *is_null = 1;
        return 0.0;
    }
    return scisql_flux2absigma(*((double *) args->args[0]),
                               *((double *) args->args[1]));
}


SCISQL_UDF_INIT(fluxToAbMagSigma)
SCISQL_REAL_UDF(fluxToAbMagSigma)


#ifdef __cplusplus
} /* extern "C" */
#endif


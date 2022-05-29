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
<udf name="${SCISQL_PREFIX}fluxToDnSigma"
     return_type="DOUBLE PRECISION"
     section="photometry">

    <desc>
        Converts a calibrated (AB) flux error to raw flux error in DN.
    </desc>
    <args>
        <arg name="flux" type="DOUBLE PRECISION"
             units="erg/cm&lt;sup&gt;2&lt;/sup&gt;/sec/Hz">
            Calibrated (AB) flux.
        </arg>
        <arg name="fluxSigma" type="DOUBLE PRECISION"
             units="erg/cm&lt;sup&gt;2&lt;/sup&gt;/sec/Hz">
            Standard deviation of flux.
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
        <note>
            If fluxMag0 is negative or zero, NULL is returned.
        </note>
        <note>
            If either fluxSigma or fluxMag0Sigma is negative, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}fluxToDnSigma(
                rFlux_PS, rFlux_PS_Sigma, 3.0e+12, 0.0)
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


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(fluxToDnSigma, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count != 4) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(fluxToDnSigma)
                 " expects exactly 4 arguments");
        return 1;
    }
    for (i = 0; i < 4; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] == 0) {
            const_item = 0;
        }
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    initid->decimals = 31;
    return 0;
}


SCISQL_API double SCISQL_VERSIONED_FNAME(fluxToDnSigma, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    double **a = (double **) args->args;
    double dnsigma;
    size_t i;
    for (i = 0; i < 4; ++i) {
        if (args->args[i] == 0 || SCISQL_ISSPECIAL(*a[i])) {
            *is_null = 1;
            return 0.0;
        }
    }
    if (*a[1] < 0.0 || *a[2] <= 0.0 || *a[3] < 0.0) {
        *is_null = 1;
        return 0.0;
    }
    dnsigma = scisql_flux2dnsigma(*a[0], *a[1], *a[2], *a[3]);
    if (SCISQL_ISSPECIAL(dnsigma)) {
        *is_null = 1;
        return 0.0;
    }
    return dnsigma;
}


SCISQL_UDF_INIT(fluxToDnSigma)
SCISQL_REAL_UDF(fluxToDnSigma)


#ifdef __cplusplus
} /* extern "C" */
#endif


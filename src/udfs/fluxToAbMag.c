/*
    Copyright (C) 2011 Serge Monkewitz

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License v3 as published
    by the Free Software Foundation, or any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    A copy of the LGPLv3 is available at <http://www.gnu.org/licenses/>.

    Authors:
        - Serge Monkewitz, IPAC/Caltech

    Work on this project has been sponsored by LSST and SLAC/DOE.
*/

/**
<udf name="fluxToAbMag" return_type="DOUBLE PRECISION" section="photometry">
    <desc>
        Converts a cailbrated (AB) flux to an AB magnitude.
    </desc>
    <args>
        <arg name="flux" type="DOUBLE PRECISION" units="erg/cm&lt;sup&gt;2&lt;/sup&gt;/sec/Hz">
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
        SELECT fluxToAbMag(uFlux_PS)
            FROM Object
            WHERE uFlux_PS IS NOT NULL
            LIMIT 10;
    </example>
</udf>
*/

#include <stdio.h>

#include "mysql.h"

#include "photometry.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool fluxToAbMag_init(UDF_INIT *initid,
                                    UDF_ARGS *args,
                                    char *message)
{
    if (args->arg_count != 1) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "fluxToAbMag() expects exactly 1 argument");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->const_item = (args->args[0] != 0);
    initid->decimals = 31;
    return 0;
}


SCISQL_API double fluxToAbMag(UDF_INIT *initid SCISQL_UNUSED,
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


#ifdef __cplusplus
} /* extern "C" */
#endif


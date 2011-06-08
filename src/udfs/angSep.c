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
<udf name="angSep" return_type="DOUBLE PRECISION" section="s2">
    <desc>
        Returns the angular separation in degrees between two
        positions on the unit sphere.

        Positions may be specified either as spherical coordinate pairs
        (lon1, lat1) and (lon2, lat2), or as 3-vectors (x1, y1, z1) and
        (x2, y2, z2) with arbitrary norm. If spherical coordinates are used,
        all arguments are assumed to be in units of degrees.
    </desc>
    <args>
        <arg name="lon1" type="DOUBLE PRECISION" units="deg">
            Longitude angle of first position.
        </arg>
        <arg name="lat1" type="DOUBLE PRECISION" units="deg">
            Latitude angle of first position.
        </arg>
        <arg name="lon2" type="DOUBLE PRECISION" units="deg">
            Longitude angle of second position.
        </arg>
        <arg name="lat2" type="DOUBLE PRECISION" units="deg">
            Latitude angle of second position.
        </arg>
    </args>
    <args>
         <arg name="x1" type="DOUBLE PRECISION">
            X coordinate of first position.
         </arg>
         <arg name="y1" type="DOUBLE PRECISION">
            Y coordinate of first position.
         </arg>
         <arg name="z1" type="DOUBLE PRECISION">
            Z coordinate of first position.
         </arg> 
         <arg name="x2" type="DOUBLE PRECISION">
            X coordinate of second position.
         </arg>
         <arg name="y2" type="DOUBLE PRECISION">
            Y coordinate of second position.
         </arg>
         <arg name="z2" type="DOUBLE PRECISION">
            Z coordinate of second position.
         </arg> 
    </args>
    <notes>
        <note>
            All arguments must be convertible to type DOUBLE PRECISION.
        </note>
        <note>
            If any argument is NULL, NaN, or +/-Inf, NULL is returned. MySQL
            does not currently support storage of IEEE special values. However,
            their presence is checked for to ensure reasonable behavior if a
            future MySQL release does end up supporting them.
        </note>
        <note>
            If spherical coordinates are passed in and either latitude
            angle is not in the [-90, 90] degree range, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT angSep(0, 0, 0, 90);
        SELECT angSep(1, 0, 0, 0, 0, 1);
        SELECT angSep(ra_PS, decl_PS, ra_SG, decl_SG) FROM Object LIMIT 10;
    </example>
</udf>
*/

#include <math.h>
#include <stdio.h>

#include "mysql.h"

#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool angSep_init(UDF_INIT *initid,
                               UDF_ARGS *args,
                               char *message)
{
    size_t i;
    my_bool maybe_null = 0, const_item = 1;
    if (args->arg_count != 4 && args->arg_count != 6) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "angSep() expects 4 or 6 arguments");
        return 1;
    }
    for (i = 0; i < args->arg_count; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->maybe_null[i] == 1) {
            maybe_null = 1;
        }
        if (args->args[i] == 0) {
            const_item = 0;
        }
    }
    initid->maybe_null = maybe_null;
    initid->const_item = const_item;
    initid->decimals = 31;
    return 0;
}


SCISQL_API double angSep(UDF_INIT *initid SCISQL_UNUSED,
                         UDF_ARGS *args,
                         char *is_null,
                         char *error SCISQL_UNUSED)
{
    double **a = (double **) args->args;
    size_t i;

    /* If any input is NULL or is not finite, the result is NULL. */
    for (i = 0; i < args->arg_count; ++i) {
        if (a[i] == 0) {
            *is_null = 1;
            return 0.0;
        }
    }
    if (args->arg_count == 4) {
       scisql_sc p1, p2;
       if (scisql_sc_init(&p1, *a[0], *a[1]) != 0 ||
           scisql_sc_init(&p2, *a[2], *a[3]) != 0) {
           *is_null = 1;
           return 0.0;
       }
       return scisql_sc_angsep(&p1, &p2);
    } else {
       scisql_v3 v1, v2;
       if (scisql_v3_init(&v1, *a[0], *a[1], *a[2]) != 0 ||
           scisql_v3_init(&v2, *a[3], *a[4], *a[5]) != 0) {
           *is_null = 1;
           return 0.0;
       }
       return scisql_v3_angsep(&v1, &v2);
    }
}


#ifdef __cplusplus
} /* extern "C" */
#endif


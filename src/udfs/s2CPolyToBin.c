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
<udf name="${SCISQL_PREFIX}s2CPolyToBin" return_type="BINARY" section="s2">
    <desc>
        Returns a binary-string representation of a spherical convex
        polygon. The polygon must be specified as a sequence of at least
        3 and at most 20 vertices. An N vertex input will result in a
        binary string of length exactly 24*(N + 1).
    </desc>
    <args varargs="true">
        <arg name="v1Lon" type="DOUBLE PRECISION" units="deg">
            Longitude angle of first polygon vertex.
        </arg>
        <arg name="v1Lat" type="DOUBLE PRECISION" units="deg">
            Latitude angle of first polygon vertex.
        </arg>
        <arg name="v2Lon" type="DOUBLE PRECISION" units="deg">
            Longitude angle of second polygon vertex.
        </arg>   
        <arg name="v2Lat" type="DOUBLE PRECISION" units="deg">
            Latitude angle of second polygon vertex.
        </arg>
    </args>
    <notes>
        <note>
            If any parameter is NULL, NaN or +/-Inf, this is an error
            and NULL is returned.
        </note>
        <note>
            If any latitude angle lies outside of [-90, 90] degrees,
            this is an error and NULL is returned.
        </note>
        <note>
            Polygon vertices can be specified in either clockwise or
            counter-clockwise order. However, the vertices are assumed to be
            hemispherical, to define edges that do not intersect except at
            vertices, and to define edges that form a convex polygon.
        </note>
        <note>
            Input coordinate must be convertible to type DOUBLE PRECISION.
            If their actual type is BIGINT or DECIMAL, then the conversion
            can result in loss of precision and hence an inaccurate result.
            Loss of precision will not occur so long as the inputs are values of
            type DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT, or TINYINT.
        </note>
    </notes>
    <example>
        CREATE TEMPORARY TABLE Poly (
            ra1  DOUBLE PRECISION NOT NULL,
            dec1 DOUBLE PRECISION NOT NULL,
            ra2  DOUBLE PRECISION NOT NULL,
            dec2 DOUBLE PRECISION NOT NULL,
            ra3  DOUBLE PRECISION NOT NULL,
            dec3 DOUBLE PRECISION NOT NULL,
            poly BINARY(96) DEFAULT NULL
        );

        INSERT INTO Poly VALUES (-10,  0,
                                  10,  0,
                                   0, 10,
                                 NULL);

        UPDATE Poly
            SET poly = ${SCISQL_PREFIX}s2CPolyToBin(
                ra1, dec1,
                ra2, dec2,
                ra3, dec3);
    </example>
</udf>
*/

#include <stdio.h>

#include "mysql.h"

#include "udf.h"
#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(s2CPolyToBin, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count < 6 ||
        args->arg_count > 2 * SCISQL_MAX_VERTS ||
        (args->arg_count & 1) != 0) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(s2CPolyToBin) 
                 " expects between 3 and %d spherical coordinate pairs",
                 SCISQL_MAX_VERTS);
        return 1;
    }
    for (i = 0; i < args->arg_count; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] == 0) {
            const_item = 0;
        }
    }
    initid->maybe_null = 1;
    initid->max_length = 3 * sizeof(double) * SCISQL_MAX_VERTS;
    initid->const_item = const_item;
    return 0;
}


SCISQL_API char * SCISQL_VERSIONED_FNAME(s2CPolyToBin, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *result,
    unsigned long *length,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    scisql_s2cpoly poly;
    scisql_sc pt;
    scisql_v3 verts[SCISQL_MAX_VERTS];
    size_t i, n;
    double **a = (double **) args->args;

    /* If any input is NULL, the result is NULL. */
    for (i = 0; i < args->arg_count; ++i) {
        if (a[i] == 0) {
            *is_null = 1;
            return result;
        }
    }
    for (i = 0, n = 0; i < args->arg_count; i += 2, ++n) {
        if (scisql_sc_init(&pt, *a[i], *a[i + 1]) != 0) {
            *is_null = 1;
            return result;
        }
        scisql_sctov3(&verts[n], &pt);
    }
    if (scisql_s2cpoly_init(&poly, verts, n) != 0) {
        *is_null = 1;
        return result;
    }

    *length = (unsigned long) scisql_s2cpoly_tobin((unsigned char *) result,
                                                   255u, &poly);
    if (*length == 0) {
        *is_null = 1;
    }
    return result;
}


SCISQL_UDF_INIT(s2CPolyToBin)
SCISQL_STRING_UDF(s2CPolyToBin)


#ifdef __cplusplus
} /* extern "C" */
#endif


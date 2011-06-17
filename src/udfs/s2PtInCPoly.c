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
<udf name="${SCISQL_PREFIX}s2PtInCPoly" return_type="INTEGER" section="s2">
    <desc>
        Returns 1 if the point (lon, lat) lies inside the given
        spherical convex polygon and 0 otherwise. The polygon may
        be specified either as a VARBINARY byte string (as produced by
        ${SCISQL_PREFIX}s2CPolyToBin()), or as a sequence of at least 3
        and at most 20 vertex pairs.
    </desc>
    <args>
        <arg name="lon" type="DOUBLE PRECISION" units="deg">
            Longitude angle of point to test.
        </arg>
        <arg name="lat" type="DOUBLE PRECISION" units="deg">
            Latitude angle of point to test.
        </arg>
        <arg name="poly" type="VARBINARY">
            Binary-string representation of polygon.
        </arg>
    </args>
    <args varargs="true">
        <arg name="lon" type="DOUBLE PRECISION" units="deg">
            Longitude angle of point to test.
        </arg>
        <arg name="lat" type="DOUBLE PRECISION" units="deg">
            Latitude angle of point to test.
        </arg>
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
            If any parameter is NULL, 0 is returned.
        </note>
        <note>
            If any coordinate is NaN or +/-Inf, this is an error and NULL is
            returned (IEEE specials are not currently supported by MySQL).
        </note>
        <note>
            If any latitude angle lies outside of [-90, 90] degrees,
            this is an error and NULL is returned.
        </note>
        <note>
            Polygon vertices can be specified in either clockwise or
            counter-clockwise order. However, vertices are assumed to be
            hemispherical, to define edges that do not intersect except at
            vertices, and to define edges that form a convex polygon.
        </note>
        <note>
            Coordinate values must be convertible to type DOUBLE PRECISION. If
            their actual types are BIGINT or DECIMAL, then the conversion can
            result in loss of precision and hence an inaccurate result. Loss of
            precision will not occur so long as the inputs are values of type
            DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT or TINYINT.
        </note>
    </notes>
    <example>
        SELECT scienceCcdExposureId
            FROM Science_Ccd_Exposure
            WHERE ${SCISQL_PREFIX}s2PtInCPoly(
                0.0, 0.0,
                llcRa, llcDecl,
                ulcRa, ulcDecl,
                urcRa, urcDecl,
                lrcRa, lrcDecl) = 1;
    </example>
</udf>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mysql.h"

#include "udf.h"
#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    int valid;
    int const_pos;
    int const_poly;
    scisql_v3 pos;
    scisql_s2cpoly poly;
} _scisql_ptpoly_state;


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(s2PtInCPoly, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    size_t i;
    my_bool const_item = 1, const_pos = 1, const_poly = 1;

    if (args->arg_count != 3) {
        if (args->arg_count < 8 ||
            args->arg_count > 2 + 2 * SCISQL_MAX_VERTS ||
            (args->arg_count & 1) != 0) {
            snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(s2PtInCPoly)
                     " expects between 4 and %d spherical coordinate pairs",
                     SCISQL_MAX_VERTS + 1);
            return 1;
        }
    } else if (args->arg_type[2] != STRING_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(s2PtInCPoly) 
                 " expects a spherical coordinate pair and a polygon"
                 " byte-string");
        return 1;
    }
    for (i = 0; i < 2; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] == 0) {
            const_item = 0;
            const_pos = 0;
        }
    }
    for (i = 2; i < args->arg_count; ++i) {
        if (args->arg_count != 3) {
            args->arg_type[i] = REAL_RESULT;
        }
        if (args->args[i] == 0) {
            const_item = 0;
            const_poly = 0;
        }
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    initid->ptr = 0;
    if (const_pos != 0 || const_poly != 0) {
        _scisql_ptpoly_state *state =
            (_scisql_ptpoly_state *) malloc(sizeof(_scisql_ptpoly_state));
        if (state != 0) {
            state->valid = 0;
            state->const_pos = const_pos;
            state->const_poly = const_poly;
            initid->ptr = (char *) state;
        }
    }
    return 0;
}


SCISQL_API long long SCISQL_VERSIONED_FNAME(s2PtInCPoly, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    _scisql_ptpoly_state s;
    _scisql_ptpoly_state *state;
    size_t i, n;
    scisql_sc pt;
    scisql_v3 verts[SCISQL_MAX_VERTS];

    s.valid = 0;
    s.const_pos = 0;
    s.const_poly = 0;
    state = (initid->ptr == 0) ? &s : (_scisql_ptpoly_state *) initid->ptr;

    if (state->valid == 0 || state->const_pos == 0) {
        /* if position isn't constant or isn't cached yet, extract it
           from the arguments and convert it to a unit vector. */
        double **a = (double **) args->args;
        if (a[0] == 0 || a[1] == 0) {
            return 0;
        }
        if (scisql_sc_init(&pt, *a[0], *a[1]) != 0) {
            *is_null = 1;
            return 0;
        }
        scisql_sctov3(&state->pos, &pt);
    }
    if (state->valid == 0 || state->const_poly == 0) {
        /* if polygon isn't constant or isn't cached yet, build one
           from the arguments. */
        for (i = 2; i < args->arg_count; ++i) {
            if (args->args[i] == 0) {
                return 0;
            }
        }
        if (args->arg_count == 3) {
            i = scisql_s2cpoly_frombin(&state->poly,
                                       (unsigned char *) args->args[2],
                                       (size_t) args->lengths[2]);
            if (i != 0) {
                *is_null = 1;
                return 0;
            }
        } else {
            double **a = (double **) args->args;
            for (i = 2, n = 0; i < args->arg_count; i += 2, ++n) {
                if (scisql_sc_init(&pt, *a[i], *a[i + 1]) != 0) {
                    *is_null = 1;
                    return 0;
                }
                scisql_sctov3(&verts[n], &pt);
            }
            if (scisql_s2cpoly_init(&state->poly, verts, n) != 0) {
                *is_null = 1;
                return 0;
            }
        }
    }
    state->valid = 1;
    return scisql_s2cpoly_cv3(&state->poly, &state->pos);
}


SCISQL_API void SCISQL_VERSIONED_FNAME(s2PtInCPoly, _deinit) (
    UDF_INIT *initid)
{
    free(initid->ptr);
}


SCISQL_UDF_INIT(s2PtInCPoly)
SCISQL_UDF_DEINIT(s2PtInCPoly)
SCISQL_INTEGER_UDF(s2PtInCPoly)


#ifdef __cplusplus
} /* extern "C" */
#endif


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
<udf name="${SCISQL_PREFIX}s2PtInCircle" return_type="INTEGER" section="s2">
    <desc>
        Returns 1 if the point (lon, lat) lies inside the given
        spherical circle and 0 otherwise.
    </desc>
    <args>
        <arg name="lon" type="DOUBLE PRECISION" units="deg">
            Longitude angle of point to test.
        </arg>
        <arg name="lat" type="DOUBLE PRECISION" units="deg">
            Latitude angle of point to test.
        </arg>
        <arg name="centerLon" type="DOUBLE PRECISION" units="deg">
            Circle center longitude angle.
        </arg>
        <arg name="centerLat" type="DOUBLE PRECISION" units="deg">
            Circle center latitude angle.
        </arg>
        <arg name="radius" type="DOUBLE PRECISION" units="deg">
            Circle radius.
        </arg>
    </args>
    <notes>
        <note>
            If any parameter is NULL, 0 is returned.
        </note>
        <note>
            If any parameter is NaN or +/-Inf, this is an error and NULL is
            returned (IEEE specials are not currently supported by MySQL).
        </note>
        <note>
            If lat or centerLat lies outside of [-90, 90] degrees, this is an
            error and NULL is returned.
        </note>
        <note>
            If radius is negative or greater than 180, this is
            an error and NULL is returned.
        </note>
        <note>
            Input values must be convertible to type DOUBLE PRECISION. If their
            actual types are BIGINT or DECIMAL, then the conversion can result
            in loss of precision and hence an inaccurate result. Loss of
            precision will not occur so long as the inputs are values of type
            DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT or TINYINT.
        </note>
    </notes>
    <example>
        SELECT objectId, ra_PS, decl_PS
            FROM Object
            WHERE ${SCISQL_PREFIX}s2PtInCircle(ra_PS, decl_PS, 0.0, 0.0, 1.0) = 1;
    </example>
</udf>
*/

#include <stdlib.h>
#include <stdio.h>

#include "mysql.h"

#include "udf.h"
#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    double dist2;
    int valid;
} _scisql_dist2_cache;


SCISQL_API SCISQL_BOOL SCISQL_VERSIONED_FNAME(s2PtInCircle, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    size_t i;
    SCISQL_BOOL const_item = 1;
    if (args->arg_count != 5) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(s2PtInCircle)
                 " expects exactly 5 arguments");
        return 1;
    }
    for (i = 0; i < 5; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] == 0) {
            const_item = 0;
        }
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    initid->ptr = 0;
    /* For constant radius circles, cache the square secant distance
       corresponding to the radius across calls. */
    if (args->args[4] != 0) {
        initid->ptr = (char *) calloc(1, sizeof(_scisql_dist2_cache));
    }
    return 0;
}


SCISQL_API long long SCISQL_VERSIONED_FNAME(s2PtInCircle, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    scisql_sc p, cen;
    double r, angle;
    double **a = (double **) args->args;
    size_t i;
    /* If any input is null, the result is 0. */
    for (i = 0; i < 5; ++i) {
        if (a[i] == 0) {
            return 0;
        }
    }
    if (scisql_sc_init(&p, *a[0], *a[1]) != 0 ||
        scisql_sc_init(&cen, *a[2], *a[3]) != 0) {
        *is_null = 1;
        return 0;
    }
    r = *a[4];
    if (r < 0.0 || r > 180.0 || SCISQL_ISNAN(r)) {
        *is_null = 1;
        return 0;
    }
    /* Fail-fast if latitude angle delta exceeds the radius. */
    if (fabs(p.lat - cen.lat) > r) {
        return 0;
    }
    if (initid->ptr == 0) {
        angle = scisql_sc_angsep(&p, &cen);
    } else {
        _scisql_dist2_cache *cache = (_scisql_dist2_cache *) initid->ptr;
        if (cache->valid == 0) {
            /* Compute square secant distance corresponding to circle.
               Avoids an asin() and sqrt() for constant radii. */
            double d;
            d = sin(r * 0.5 * SCISQL_RAD_PER_DEG);
            cache->dist2 = 4.0 * d * d;
            cache->valid = 1;
        }
        r = cache->dist2;
        angle = scisql_sc_dist2(&p, &cen);
    }
    return angle <= r;
}


SCISQL_API void SCISQL_VERSIONED_FNAME(s2PtInCircle, _deinit) (
    UDF_INIT *initid)
{
    free(initid->ptr);
}


SCISQL_UDF_INIT(s2PtInCircle)
SCISQL_UDF_DEINIT(s2PtInCircle)
SCISQL_INTEGER_UDF(s2PtInCircle)


#ifdef __cplusplus
} /* extern "C" */
#endif


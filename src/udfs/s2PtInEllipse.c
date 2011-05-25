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
<udf name="s2PtInEllipse" return_type="INTEGER" section="s2">
    <desc>
        Returns 1 if the point (lon, lat) lies inside the given
        spherical ellipse and 0 otherwise.
    </desc>
    <args>
        <arg name="lon" type="DOUBLE PRECISION" units="deg">
            Longitude angle of point to test.
        </arg>
        <arg name="lat" type="DOUBLE PRECISION" units="deg">
            Latitude angle of point to test.
        </arg>
        <arg name="centerLon" type="DOUBLE PRECISION" units="deg">
            Ellipse center longitude angle.
        </arg>
        <arg name="centerLat" type="DOUBLE PRECISION" units="deg">
            Ellipse center latitude angle.
        </arg>
        <arg name="semiMajorAxisAngle" type="DOUBLE PRECISION" units="arcsec">
            Semi-major axis length.
        </arg>
        <arg name="semiMinorAxisAngle" type="DOUBLE PRECISION" units="arcsec">
            Semi-minor axis length.
        </arg>
        <arg name="positionAngle" type="DOUBLE PRECISION" units="deg">
            Ellipse position angle, east of north.
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
            If semiMinorAxisAngle is negative or greater than
            semiMajorAxisAngle, this is an error and NULL is returned.
        </note>
        <note>
            If semiMajorAxisAngle is greater than 36,000 arcsec (10 deg),
            this is an error and NULL is returned.
        </note>
        <note>
            All inputs must be convertible to type DOUBLE PRECISION. If their
            actual types are BIGINT or DECIMAL, then the conversion can result
            in loss of precision and hence an inaccurate result. Loss of
            precision will not occur so long as the inputs are values of type
            DOUBLE PRECISION, FLOAT, REAL, INTEGER, SMALLINT or TINYINT.
        </note>
    </notes>
    <example>
        SELECT objectId, ra_PS, decl_PS
            FROM Object
            WHERE s2PtInEllipse(ra_PS, decl_PS, 0, 0, 10, 5, 90) = 1;
    </example>
</udf>
*/

#include <stdlib.h>
#include <stdio.h>

#include "mysql.h"

#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    double sinLon;    /* sine of ellipse center longitude */
    double cosLon;    /* cosine of ellipse center longitude */
    double sinLat;    /* sine of ellipse center latitude */
    double cosLat;    /* cosine of ellipse center latitude */
    double sinPosAng; /* sine of ellipse position angle */
    double cosPosAng; /* cosine of ellipse position angle */
    double invMinor2; /* 1/(m*m); m = semi-minor axis length (rad) */
    double invMajor2; /* 1/(M*M); M = semi-major axis length (rad) */
    int valid;
} _scisql_s2ellipse;


SCISQL_API my_bool s2PtInEllipse_init(UDF_INIT *initid,
                                      UDF_ARGS *args,
                                      char *message)
{
    int i;
    my_bool const_item = 1, const_ellipse = 1;
    if (args->arg_count != 7) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "s2PtInEllipse() expects exactly 7 arguments");
        return 1;
    }
    for (i = 0; i < 7; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] == 0) {
            const_item = 0;
            if (i >= 2) {
                const_ellipse = 0;
            }
        }
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    initid->ptr = 0;
    /* If ellipse parameters are constant, allocate derived quantity cache. */
    if (const_ellipse) {
        initid->ptr = (char *) calloc(1, sizeof(_scisql_s2ellipse));
    }
    return 0;
}


SCISQL_API long long s2PtInEllipse(UDF_INIT *initid,
                                   UDF_ARGS *args,
                                   char *is_null,
                                   char *error SCISQL_UNUSED)
{
    _scisql_s2ellipse ellipse;
    scisql_sc p, cen;
    scisql_v3 v;
    double xne, yne, x, y;
    _scisql_s2ellipse *ep;
    double **a = (double **) args->args;
    int i;

    /* If any input is null, the result is 0. */
    for (i = 0; i < 7; ++i) {
        if (args->args[i] == 0) {
            return 0;
        }
    }
    if (scisql_sc_init(&p, *a[0], *a[1]) != 0) {
        *is_null = 1;
        return 0;
    }

    ellipse.valid = 0;
    ep = (initid->ptr != 0) ? (_scisql_s2ellipse *) initid->ptr : &ellipse;
    if (ep->valid == 0) {
        double M = *a[4];
        double m = *a[5];
        double posang = *a[6] * SCISQL_RAD_PER_DEG;
        if (SCISQL_ISSPECIAL(posang) || SCISQL_ISNAN(M) || SCISQL_ISNAN(m)) {
            *is_null = 1;
            return 0;
        }
        /* Semi-minor axis length m and semi-major axis length M must satisfy
           0 <= m <= M <= 10 deg */
        if (m < 0.0 || m > M || M > 10.0 * SCISQL_ARCSEC_PER_DEG) {
            *is_null = 1;
            return 0;
        }
        if (scisql_sc_init(&cen, *a[2], *a[3]) != 0) {
            *is_null = 1;
            return 0;
        }
        ep->sinLon = sin(cen.lon * SCISQL_RAD_PER_DEG);
        ep->cosLon = cos(cen.lon * SCISQL_RAD_PER_DEG);
        ep->sinLat = sin(cen.lat * SCISQL_RAD_PER_DEG);
        ep->cosLat = cos(cen.lat * SCISQL_RAD_PER_DEG);
        ep->sinPosAng = sin(posang);
        ep->cosPosAng = cos(posang);
        m = m * SCISQL_RAD_PER_DEG / SCISQL_ARCSEC_PER_DEG;
        M = M * SCISQL_RAD_PER_DEG / SCISQL_ARCSEC_PER_DEG;
        ep->invMinor2 = 1.0 / (m * m);
        ep->invMajor2 = 1.0 / (M * M);
        ep->valid = 1;
    }
    /* Transform input position from spherical coordinates
       to a unit cartesian vector. */
    scisql_sctov3(&v, &p);
    /* get coords of input point in (N,E) basis at ellipse center */
    xne = ep->cosLat * v.z - ep->sinLat * (ep->sinLon * v.y + ep->cosLon * v.x);
    yne = ep->cosLon * v.y - ep->sinLon * v.x;
    /* rotate by negated position angle */
    x = ep->sinPosAng * yne + ep->cosPosAng * xne;
    y = ep->cosPosAng * yne - ep->sinPosAng * xne;
    /* perform standard 2D axis-aligned point-in-ellipse test */
    return (x * x * ep->invMajor2 + y * y * ep->invMinor2 <= 1.0);
}


SCISQL_API void s2PtInEllipse_deinit(UDF_INIT *initid) {
    free(initid->ptr);
}


#ifdef __cplusplus
} /* extern "C" */
#endif


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
        - Fritz Mueller, SLAC National Accelerator Laboratory

    Work on this project has been sponsored by LSST and SLAC/DOE.

    ----------------------------------------------------------------

    Photometry related routines, currently limited to converting raw DN to
    calibrated fluxes and AB magnitudes.
*/

#ifndef SCISQL_PHOTOMETRY_H
#define SCISQL_PHOTOMETRY_H
#include <math.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


/* 2.5 / log(10) */
#define SCISQL_5_OVER_2LOG10 1.085736204758129569
/* log(10) / 2.5 */
#define SCISQL_2LOG10_OVER_5 0.9210340371976182736
/* pow(10, -48.6/2.5) */
#define SCISQL_AB_FLUX_SCALE 3.630780547701013425e-20


SCISQL_INLINE double scisql_hypot(double a, double b) {
    double q;
    if (fabs(a) < fabs(b)) {
        double c = a;
        a = b;
        b = c;
    }
    if (a == 0.0) {
        return 0.0;
    }
    q = b / a;
    return fabs(a) * sqrt(1.0 + q*q);
}

/*  Converts a calibrated flux (erg/cm**2/sec/Hz) to an AB magnitude.
 */
SCISQL_INLINE double scisql_flux2ab(double flux) {
    return -2.5 * log10(flux) - 48.6;
}

/*  Converts calibrated flux error (erg/cm**2/sec/Hz) to an AB magnitude error.
 */
SCISQL_INLINE double scisql_flux2absigma(double flux, double fluxsigma) {
    return SCISQL_5_OVER_2LOG10 * fluxsigma / flux;
}

/*  Converts a calibrated flux (nanojansky) to an AB magnitude.
 */
SCISQL_INLINE double scisql_nanojansky2ab(double flux) {
    return -2.5 * log10(flux) + 31.4;
}

/*  Converts calibrated flux error (nanojansky) to an AB magnitude error.
 */
SCISQL_INLINE double scisql_nanojansky2absigma(double flux, double fluxsigma) {
    return SCISQL_5_OVER_2LOG10 * fluxsigma / flux;
}

/*  Converts a raw DN value to a calibrated flux (erg/cm**2/sec/Hz).
 */
SCISQL_INLINE double scisql_dn2flux(double dn, double fluxmag0) {
    return SCISQL_AB_FLUX_SCALE * dn / fluxmag0; 
}

/*  Converts a raw DN value to a calibrated flux (erg/cm**2/sec/Hz).
 */
SCISQL_INLINE double scisql_dn2fluxsigma(double dn,
                                         double dnsigma,
                                         double fluxmag0,
                                         double fluxmag0sigma)
{
    return SCISQL_AB_FLUX_SCALE *
           scisql_hypot(dn * fluxmag0sigma, dnsigma * fluxmag0) /
           (fluxmag0*fluxmag0);
}

/*  Converts a raw DN value to an AB magnitude.
 */
SCISQL_INLINE double scisql_dn2ab(double dn, double fluxmag0) {
    return scisql_flux2ab(scisql_dn2flux(dn, fluxmag0)); 
}

/*  Converts a raw DN error to an AB magnitude error.
 */
SCISQL_INLINE double scisql_dn2absigma(double dn,
                                       double dnsigma,
                                       double fluxmag0,
                                       double fluxmag0sigma)
{
    return scisql_flux2absigma(
        scisql_dn2flux(dn, fluxmag0),
        scisql_dn2fluxsigma(dn, dnsigma, fluxmag0, fluxmag0sigma));
}

/*  Converts a calibrated flux (erg/cm**2/sec/Hz) to a raw DN value.
 */
SCISQL_INLINE double scisql_flux2dn(double flux, double fluxmag0) {
    return flux * fluxmag0 / SCISQL_AB_FLUX_SCALE;
}

/*  Converts a calibrated flux error (erg/cm**2/sec/Hz) to a raw DN error.
 */
SCISQL_INLINE double scisql_flux2dnsigma(double flux,
                                         double fluxsigma,
                                         double fluxmag0,
                                         double fluxmag0sigma) {
    return scisql_hypot(flux * fluxmag0sigma, fluxmag0 * fluxsigma) /
           SCISQL_AB_FLUX_SCALE;
}

/*  Converts an AB magnitude to a calibrated flux (erg/cm**2/sec/Hz).
 */
SCISQL_INLINE double scisql_ab2flux(double mag) {
    return pow(10.0, -0.4*(mag + 48.6));
}

/*  Converts an AB magnitude error to a calibrated flux error 
    (erg/cm**2/sec/Hz).
 */
SCISQL_INLINE double scisql_ab2fluxsigma(double mag, double magsigma) {
    return magsigma * scisql_ab2flux(mag) * SCISQL_2LOG10_OVER_5;
}

/*  Converts an AB magnitude to a calibrated flux (nanojansky).
 */
SCISQL_INLINE double scisql_ab2nanojansky(double mag) {
    return pow(10.0, -0.4*(mag - 31.4));
}

/*  Converts an AB magnitude error to a calibrated flux error 
    (nanojanksy).
 */
SCISQL_INLINE double scisql_ab2nanojanskysigma(double mag, double magsigma) {
    return magsigma * scisql_ab2nanojansky(mag) * SCISQL_2LOG10_OVER_5;
}

/*  Converts an AB magnitude to a raw DN value.
 */
SCISQL_INLINE double scisql_ab2dn(double mag, double fluxmag0) {
    return scisql_flux2dn(scisql_ab2flux(mag), fluxmag0);
}

/*  Converts an AB magnitude error to a raw DN error.
 */
SCISQL_INLINE double scisql_ab2dnsigma(double mag,
                                       double magsigma,
                                       double fluxmag0,
                                       double fluxmag0sigma) {
    double flux = scisql_ab2flux(mag);
    return scisql_flux2dnsigma(
        flux,
        magsigma * flux * SCISQL_2LOG10_OVER_5,
        fluxmag0,
        fluxmag0sigma);
}


#ifdef __cplusplus
}
#endif

#endif /* SCISQL_PHOTOMETRY_H */


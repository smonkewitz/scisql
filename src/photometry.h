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
#define SCISQL_AB_FLUX_SCALE 3.630780547701013425e-20

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
    double d = dn * fluxmag0sigma / fluxmag0;
    return SCISQL_AB_FLUX_SCALE * sqrt((dnsigma*dnsigma + d*d) /
                                       (fluxmag0*fluxmag0));
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


#ifdef __cplusplus
}
#endif

#endif /* SCISQL_PHOTOMETRY_H */


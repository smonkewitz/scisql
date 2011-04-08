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

    A minimalistic set of functions and types for HTM indexing.

    This software is based on work by A. Szalay, T. Budavari,
    G. Fekete at The Johns Hopkins University, and Jim Gray,
    Microsoft Research. See the following links for more information:

    http://voservices.net/spherical/
    http://adsabs.harvard.edu/abs/2010PASP..122.1375B
 */
#ifndef SCISQL_HTM_H
#define SCISQL_HTM_H

#include <stdint.h>

#include "common.h"
#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Maximum HTM tree subdivision level */
#define SCISQL_HTM_MAX_LEVEL 24

/*  A sorted list of 64 bit integer ranges. 
 */
typedef struct {
  size_t n;         /* number of ranges in list */
  size_t cap;       /* capacity of the range list */
  int64_t ranges[]; /* ranges [min_i, max_i], with min_i <= max_i
                       and min_j > max_i for all j > i. */
} scisql_ids;

/*  Computes an HTM ID for a position.

    Returns -1 if v is 0 or level is not in [0, SCISQL_HTM_MAX_LEVEL].
    Valid IDs are always positive.
 */
SCISQL_LOCAL int64_t scisql_v3_htmid(const scisql_v3 *unit_v, int level);

/*  Computes HTM IDs for a list of positions. Positions are sorted
    by HTM ID during the id generation process.

    Returns 0 on success and 1 on error.
 */
SCISQL_LOCAL int scisql_v3_htmsort(scisql_v3 *unit_vecs,
                                   int64_t *ids,
                                   size_t n,
                                   int level);

/*
 */
SCISQL_LOCAL scisql_ids * scisql_s2circle_htmids(const scisql_v3 *center,
                                                 double radius,
                                                 int level);

/*
 */
SCISQL_LOCAL scisql_ids * scisql_s2cpoly_htmids(const scisql_s2cpoly *poly,
                                                int level);

#ifdef __cplusplus
}
#endif

#endif /* SCISQL_HTM_H */


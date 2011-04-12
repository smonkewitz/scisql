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

    Globally interesting declarations/macros.
  */
#ifndef SCISQL_COMMON_H
#define SCISQL_COMMON_H

#include <stddef.h>
#include <math.h>
#include "config.h"


/*  Visibility macros.

    SCISQL_API: explicitly marks a function as part of the scisql API.
    SCISQL_LOCAL: explicitly marks a function as internal to the scisql implementation.

    The intent is to use these macros to hide any implementation functions not required
    for MySQL to call the scisql UDFs.
*/
#if defined _WIN32 || defined __CYGWIN__
#   define SCISQL_API __declspec(dllexport)
#   define SCISQL_LOCAL
#else
#   if HAVE_ATTRIBUTE_VISIBILITY
#       define SCISQL_API __attribute__ ((visibility("default")))
#       define SCISQL_LOCAL __attribute__ ((visibility("hidden")))
#   else
#       define SCISQL_API
#       define SCISQL_LOCAL
#   endif
#endif

/*  Marking function arguments as unused
 */
#if HAVE_ATTRIBUTE_UNUSED
#   define SCISQL_UNUSED __attribute__ ((unused))
#else
#   define SCISQL_UNUSED
#endif

/*  Inline function support
 */
#if __STDC_VERSION__ >= 199901L || __GNUC__
/* "inline" is a keyword */
#   define SCISQL_INLINE static inline
#else
#   define SCISQL_INLINE static SCISQL_UNUSED
#endif

/*  Alignment support
 */
#if HAVE_ATTRIBUTE_ALIGNED
#   define SCISQL_ALIGNED(x) __attribute__ ((aligned(x)))
#else
#   define SCISQL_ALIGNED(x)
#endif

/*  Testing for IEEE specials
 */
#if __STDC_VERSION__ >= 199901L
#   define SCISQL_ISNAN(x) isnan(x)
#   define SCISQL_ISSPECIAL(x) (!isfinite(x)) 
#else
#   define SCISQL_ISNAN(x) ((x) != (x))
#   define SCISQL_ISSPECIAL(x) ((x) != (x) || ((x) != 0.0 && (x) == 2*(x)))
#endif

#endif /* SCISQL_COMMON_H */

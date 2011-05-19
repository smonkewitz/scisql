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
#include <stdio.h>

#include "mysql.h"

#include "photometry.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool dnToAbMagSigma_init(UDF_INIT *initid,
                                       UDF_ARGS *args,
                                       char *message)
{
    size_t i;
    my_bool const_item = 1;
    if (args->arg_count != 4) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "dnToAbMagSigma() expects exactly 4 arguments");
        return 1;
    }
    for (i = 0; i < 4; ++i) {
        args->arg_type[i] = REAL_RESULT;
        if (args->args[i] != 0) {
            const_item = 0;
        } 
    }
    initid->maybe_null = 1;
    initid->const_item = const_item;
    initid->decimals = 31;
    return 0;
}


SCISQL_API double dnToAbMagSigma(UDF_INIT *initid SCISQL_UNUSED,
                                 UDF_ARGS *args,
                                 char *is_null,
                                 char *error SCISQL_UNUSED)
{
    double **a = (double **) args->args;
    size_t i;
    for (i = 0; i < 4; ++i) {
        if (args->args[i] == 0) {
            *is_null = 1;
            return 0.0;
        }
    }
    return scisql_dn2absigma(*a[0], *a[1], *a[2], *a[3]);
}


#ifdef __cplusplus
} /* extern "C" */
#endif


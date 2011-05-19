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


SCISQL_API my_bool dnToAbMag_init(UDF_INIT *initid,
                                  UDF_ARGS *args,
                                  char *message)
{
    if (args->arg_count != 2) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "dnToAbMag() expects exactly 2 arguments");
        return 1;
    }
    args->arg_type[0] = REAL_RESULT;
    args->arg_type[1] = REAL_RESULT;
    initid->maybe_null = 1;
    initid->const_item = (args->args[0] != 0 && args->args[1] != 0);
    initid->decimals = 31;
    return 0;
}


SCISQL_API double dnToAbMag(UDF_INIT *initid SCISQL_UNUSED,
                            UDF_ARGS *args,
                            char *is_null,
                            char *error SCISQL_UNUSED)
{
    if (args->args[0] == 0 || args->args[1] == 0) {
        *is_null = 1;
        return 0.0;
    }
    return scisql_dn2ab(*((double *) args->args[0]),
                        *((double *) args->args[1]));
}


#ifdef __cplusplus
} /* extern "C" */
#endif


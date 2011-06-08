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
    ================================================================
*/

/**
<udf name="scisqlVersion" return_type="CHAR" section="misc">
    <desc>
        Returns the version of the sciSQL library in use.
    </desc>
    <args />
    <example>
        SELECT scisqlVersion();
    </example>
</udf>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mysql.h"

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool scisqlVersion_init(UDF_INIT *initid,
                                      UDF_ARGS *args,
                                      char *message)
{
    if (args->arg_count != 0) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "scisqlVersion() expects no arguments");
        return 1;
    }
    if (SCISQL_VERSION_STRING_LENGTH > 255) {
        initid->ptr = malloc(SCISQL_VERSION_STRING_LENGTH + 1);
        if (initid->ptr == 0) {
            snprintf(message, MYSQL_ERRMSG_SIZE,
                     "scisqlVersion(): memory allocation failed");
            return 1;
        }
    }
    initid->maybe_null = 0;
    initid->const_item = 1;
    return 0;
}


SCISQL_API char * scisqlVersion(UDF_INIT *initid,
                                UDF_ARGS *args SCISQL_UNUSED,
                                char *result,
                                unsigned long *length,
                                char *is_null SCISQL_UNUSED,
                                char *error SCISQL_UNUSED)
{
    char *r = (SCISQL_VERSION_STRING_LENGTH > 255) ? initid->ptr : result;
    *length = SCISQL_VERSION_STRING_LENGTH;
    strcpy(r, SCISQL_VERSION_STRING);
    return r;
}


SCISQL_API void scisqlVersion_deinit(UDF_INIT *initid) {
    if (SCISQL_VERSION_STRING_LENGTH > 255) {
        free(initid->ptr);
    }
}


#ifdef __cplusplus
} /* extern "C" */
#endif


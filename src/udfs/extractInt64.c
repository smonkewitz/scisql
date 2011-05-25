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
<udf name="extractInt64" return_type="BIGINT" section="misc" internal="true">
    <desc>
        Extracts a 64-bit integer stored in host byte order
        from a binary string.
    </desc>
    <args>
        <arg name="data" type="STRING">
            Byte string to extract a 64-bit integer from.
        </arg>
        <arg name="idx" type="INTEGER">
            The index of the 64-bit integer to extract.
        </arg>
    </args>
    <notes>
        <note>
            If any argument is NULL, NULL is returned.
        </note>
        <note>
            If idx is negative or out of range, NULL is returned.
        </note>
    </notes>
</udf>
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mysql.h"

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef union {
    int64_t value;
    unsigned char bytes[sizeof(int64_t)];
} _scisql_int64_bytes;


SCISQL_API my_bool extractInt64_init(UDF_INIT *initid,
                                     UDF_ARGS *args,
                                     char *message)
{
    if (args->arg_count != 2) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 "extractInt64() expects 2 arguments");
        return 1;
    }
    initid->const_item = (args->args[0] == 0 || args->args[1] == 0) ? 0 : 1;
    if (args->arg_type[0] != STRING_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE, "First argument of "
                 "extractInt64() must be a binary string");
        return 1;
    }
    if (args->arg_type[1] != INT_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE, "Second argument of "
                 "extractInt64() must be an integer");
        return 1;
    }
    initid->maybe_null = 1;
    return 0;
}


SCISQL_API long long extractInt64(UDF_INIT *initid SCISQL_UNUSED,
                                  UDF_ARGS *args,
                                  char *is_null,
                                  char *error SCISQL_UNUSED)
{
    _scisql_int64_bytes ibytes;
    unsigned long len;
    long long i;

    /* If any input is null, the result is null */
    if (args->args[0] == 0 || args->args[1] == 0) {
        *is_null = 1;
        return 0;
    }
    len = args->lengths[0];
    i = *((long long*) args->args[1]);
    /* If index is out of range, the result is null */
    if (i < 0 || i >= (long long) (len/sizeof(int64_t))) {
        *is_null = 1;
        return 0;
    }
    memcpy(ibytes.bytes, args->args[0] + i*sizeof(int64_t), sizeof(int64_t));
    return (long long) ibytes.value;
}


#ifdef __cplusplus
} /* extern "C" */
#endif

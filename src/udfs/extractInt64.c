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
<udf name="${SCISQL_PREFIX}extractInt64"
     return_type="BIGINT"
     section="misc" internal="true">

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

#include "udf.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef union {
    int64_t value;
    unsigned char bytes[sizeof(int64_t)];
} _scisql_int64_bytes;


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(extractInt64, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 2) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(extractInt64) " expects 2 arguments");
        return 1;
    }
    initid->const_item = (args->args[0] == 0 || args->args[1] == 0) ? 0 : 1;
    if (args->arg_type[0] != STRING_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE, "First argument of "
                 SCISQL_UDF_NAME(extractInt64) " must be a binary string");
        return 1;
    }
    if (args->arg_type[1] != INT_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE, "Second argument of "
                 SCISQL_UDF_NAME(extractInt64) " must be an integer");
        return 1;
    }
    initid->maybe_null = 1;
    return 0;
}


SCISQL_API long long SCISQL_VERSIONED_FNAME(extractInt64, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
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


SCISQL_UDF_INIT(extractInt64)
SCISQL_INTEGER_UDF(extractInt64)


#ifdef __cplusplus
} /* extern "C" */
#endif

/*
    Copyright (C) 2011 Serge Monkewitz

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
<udf name="${SCISQL_PREFIX}getVersion" return_type="CHAR" section="misc">
    <desc>
        Returns the version of the sciSQL library in use.
    </desc>
    <args />
    <example>
        SELECT ${SCISQL_PREFIX}getVersion();
    </example>
</udf>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mysql.h"

#include "udf.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool SCISQL_FNAME(getVersion, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 0) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(getVersion) " expects no arguments");
        return 1;
    }
    if (SCISQL_VERSION_STRING_LENGTH > 255) {
        initid->ptr = malloc(SCISQL_VERSION_STRING_LENGTH + 1);
        if (initid->ptr == 0) {
            snprintf(message, MYSQL_ERRMSG_SIZE,
                     SCISQL_UDF_NAME(getVersion) " memory allocation failed");
            return 1;
        }
    }
    initid->maybe_null = 0;
    initid->const_item = 1;
    return 0;
}


SCISQL_API char * SCISQL_FNAME(getVersion, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid,
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


SCISQL_API void SCISQL_FNAME(getVersion, _deinit) (
    UDF_INIT *initid)
{
    if (SCISQL_VERSION_STRING_LENGTH > 255) {
        free(initid->ptr);
    }
}


#ifdef __cplusplus
} /* extern "C" */
#endif


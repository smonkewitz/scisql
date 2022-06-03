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
<udf name="${SCISQL_PREFIX}raiseError"
     return_type="BIGINT"
     section="misc" internal="true">

    <desc>
        Fails with an optional error message.
        <p>
            This UDF exists solely because MySQL 5.1 does not support
            SIGNAL in stored procedures. The error messages it produces
            are slightly more readable than the results of hacks like
            <tt>SELECT * FROM `Lorem ipsum dolor`</tt>.
        </p>
    </desc>
    <args />
    <args>
        <arg name="message" type="STRING">Error message.</arg>
    </args>
    <example test="false">
        SELECT ${SCISQL_PREFIX}raiseError('Lorem ipsum dolor');
    </example>
</udf>
*/

#include <string.h>

#include "mysql.h"

#include "udf.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API SCISQL_BOOL SCISQL_VERSIONED_FNAME(raiseError, _init) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *message)
{
    const char *m = "A scisql related error occurred";
    if (args->arg_count > 0 && args->arg_type[0] == STRING_RESULT &&
        args->args[0] != 0 && args->lengths[0] != 0) {
        m = args->args[0];
    }
    strncpy(message, m, MYSQL_ERRMSG_SIZE - 1);
    message[MYSQL_ERRMSG_SIZE - 1] = '\0';
    return 1;
}


SCISQL_API long long SCISQL_VERSIONED_FNAME(raiseError, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args SCISQL_UNUSED,
    char *is_null SCISQL_UNUSED,
    char *error SCISQL_UNUSED)
{
    // never reached
    return 0;
}


SCISQL_UDF_INIT(raiseError)
SCISQL_INTEGER_UDF(raiseError)


#ifdef __cplusplus
} /* extern "C" */
#endif


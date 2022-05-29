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
<udf name="${SCISQL_PREFIX}s2HtmLevel" return_type="INTEGER" section="s2">
    <desc>
        Returns the subdivision level of the given HTM ID.
    </desc>
    <args>
        <arg name="id" type="BIGINT">
            HTM ID.
        </arg>
    </args>
    <notes>
        <note>
            If id is NULL or an invalid HTM ID, NULL is returned.
        </note>
    </notes>
    <example>
        SELECT ${SCISQL_PREFIX}s2HtmLevel(32);
    </example>
</udf>
*/

#include <stdlib.h>
#include <stdio.h>

#include "mysql.h"

#include "udf.h"
#include "htm.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool SCISQL_VERSIONED_FNAME(s2HtmLevel, _init) (
    UDF_INIT *initid,
    UDF_ARGS *args,
    char *message)
{
    if (args->arg_count != 1) {
        snprintf(message, MYSQL_ERRMSG_SIZE,
                 SCISQL_UDF_NAME(s2HtmId) " expects exactly 1 argument");
        return 1;
    }
    if (args->arg_type[0] != INT_RESULT) {
        snprintf(message, MYSQL_ERRMSG_SIZE, SCISQL_UDF_NAME(s2HtmLevel)
                 " HTM ID must be an integer");
    }
    initid->maybe_null = 1;
    initid->const_item = (args->args[0] != 0);
    return 0;
}


SCISQL_API long long SCISQL_VERSIONED_FNAME(s2HtmLevel, SCISQL_NO_SUFFIX) (
    UDF_INIT *initid SCISQL_UNUSED,
    UDF_ARGS *args,
    char *is_null,
    char *error SCISQL_UNUSED)
{
    int64_t id;
    int level;
    if (args->args[0] == 0) {
        *is_null = 1;
        return 0;
    }
    id = (int64_t) (*(long long *) args->args[0]);
    level = scisql_htm_level(id);
    if (level < 0) {
        *is_null = 1;
        return 0;
    }
    return level;
}


SCISQL_UDF_INIT(s2HtmLevel)
SCISQL_INTEGER_UDF(s2HtmLevel)


#ifdef __cplusplus
} /* extern "C" */
#endif


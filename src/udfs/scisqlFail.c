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
<udf name="scisqlFail" return_type="BIGINT" section="misc" internal="true">
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
    <example>
        SELECT scisqlFail('Lorem ipsum dolor');
    </example>
</udf>
*/

#include <string.h>

#include "mysql.h"

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


SCISQL_API my_bool scisqlFail_init(UDF_INIT *initid SCISQL_UNUSED,
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


SCISQL_API long long scisqlFail(UDF_INIT *initid SCISQL_UNUSED,
                                UDF_ARGS *args SCISQL_UNUSED,
                                char *is_null SCISQL_UNUSED,
                                char *error SCISQL_UNUSED)
{
    // never reached
    return 0;
}


#ifdef __cplusplus
} /* extern "C" */
#endif


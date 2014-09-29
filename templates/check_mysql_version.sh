#!/usr/bin/env sh

DIR=$(cd "$(dirname "$0")"; pwd -P)                                                                     

SCISQL_BASE={{SCISQL_BASE}}
export MYSQL_CNF=${DIR}/my-client.cnf
export MYSQL_BIN={{MYSQL_BIN}}

if [ -x "${MYSQL_BIN}" ]; then
    # mysql_config returns client version, whereas above command returns server version
    MYSQL_VERSION=`echo "SELECT VERSION()" | ${MYSQL_BIN} --defaults-file=${MYSQL_CNF} -N`
    retcode=$?
else
    >&2 echo "Invalid MySQL binary : ${MYSQL_BIN}"
    retcode=1
fi

if [ ${retcode} -eq 0 ]; then
    echo "MySQL version : ${MYSQL_VERSION}"
    python ${SCISQL_BASE}/tools/mysqlversion.py --mysqlversion "${MYSQL_VERSION}"
    retcode=$?
fi

if [ ${retcode} -eq 0 ]; then
    echo "MySQL version compatibility check SUCCESSFUL"
else
    >&2 echo "MySQL version compatibility check FAILED"
fi
exit ${retcode}

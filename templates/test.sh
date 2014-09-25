#!/usr/bin/env sh

set -e
DIR=$(cd "$(dirname "$0")"; pwd -P)                                                                     

export SCISQL_PREFIX=${SCISQL_PREFIX}
export MYSQL_CNF=${DIR}/my-client.cnf
export MYSQL=${MYSQL_BIN}

for testfile in ${SCISQL_BASE}/test/test*.py ${SCISQL_BASE}/tools/docs.py
do
    echo "Running ${testfile}"
    python ${testfile}
    echo "Test SUCCESSFULL"
done

echo "sciSQL deployment tests SUCCESSFUL"

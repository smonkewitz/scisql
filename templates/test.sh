#!/usr/bin/env sh

set -e
DIR=$(cd "$(dirname "$0")"; pwd -P)                                                                     

export SCISQL_PREFIX=${SCISQL_PREFIX}
export MYSQL_CNF=${DIR}/my-client.cnf

for testfile in ${SCISQL_BASE}/test/test*.py
do
    echo "Running ${testfile}"
    python ${testfile}
    echo "Test SUCCESSFULL"
done

echo "sciSQL deployment tests SUCCESSFUL"

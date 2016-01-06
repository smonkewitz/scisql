#!/usr/bin/env sh

set -e
DIR=$(cd "$(dirname "$0")"; pwd -P)                                                                     

MYSQL_CNF=${DIR}/my-client.cnf
MYSQL_BIN={{MYSQL_BIN}}

for file in "deploy" "demo"
do
    SQL_FILE="${DIR}/${file}.mysql"
    echo "Loading ${SQL_FILE}"
    ${MYSQL_BIN} --defaults-file=${MYSQL_CNF} < ${SQL_FILE}
done

echo "sciSQL procedures loading in MySQL/MariaDB SUCCESSFUL"

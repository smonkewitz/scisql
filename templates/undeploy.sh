#!/usr/bin/env sh

set -e
DIR=$(cd "$(dirname "$0")"; pwd -P)                                                                     

SCISQL_BASE={{SCISQL_BASE}}
export SCISQL_PREFIX={{SCISQL_PREFIX}}
export SCISQL_VSUFFIX={{SCISQL_VSUFFIX}}
export SCISQL_LIBNAME={{SCISQL_LIBNAME}}
export MYSQL_CNF=${DIR}/my-client.cnf

python ${SCISQL_BASE}/tools/undeploy.py

echo "sciSQL undeployment SUCCESSFUL"

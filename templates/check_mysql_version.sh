#!/usr/bin/env sh

DIR=$(cd "$(dirname "$0")"; pwd -P)                                                                     

export MYSQL_CNF=${DIR}/my-client.cnf
export MYSQL=${MYSQL_BIN}

retcode=0
if [[ -x ${MYSQL_BIN} ]]; then
    # mysql_config returns client version, whereas above command returns server version
    MYSQL_VERSION=`echo "SELECT VERSION()" | ${MYSQL} --defaults-file=/tmp/tmpzQW0Kf-scisql/my-client.cnf -N`
else
    ${retcode}=1    
fi

if [[ ${retcode} -eq 0 ]]; then
    echo "MySQL version compatibility check SUCCESSFUL"
else
    echo "MySQL version compatibility check FAILED"
fi
exit ${retcode}

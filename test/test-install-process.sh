# pre-requisites :
# start mysql instance (with write access on plugin/ directory)
# add mysqlpython to PYTHONPATH
# set INSTALL_DIR and PASSWORD
set -e
INSTALL_DIR="$HOME/tmp/scisql"
PASSWORD="changeme"
MYSQL_SOCKET="$HOME/qserv-run/2014_09.0/var/lib/mysql/mysql.sock"

# Testing client install
rm -rf build/* ${INSTALL_DIR}/*
./configure --client-only --prefix=${INSTALL_DIR} 
make
make install

# Testing whole install
rm -rf build/* ${INSTALL_DIR}/*
./configure --mysql-includes=${MYSQL_DIR}/include/mysql/ --prefix=${INSTALL_DIR}
make 
make install
echo $PASSWORD | PYTHONPATH="${PYTHONPATH}:${INSTALL_DIR}/python" ${INSTALL_DIR}/bin/scisql-deploy.py --mysql-socket=${MYSQL_SOCKET} -v=DEBUG
echo $PASSWORD | PYTHONPATH="${PYTHONPATH}:${INSTALL_DIR}/python" ${INSTALL_DIR}/bin/scisql-deploy.py --mysql-socket=${MYSQL_SOCKET} -v=DEBUG --undeploy
make html_docs

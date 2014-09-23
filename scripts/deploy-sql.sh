set -e
DIR=$(cd "$(dirname "$0")"; pwd -P)                                                                     

for file in "deploy" "demo"
do                         
    ${MYSQL_BIN} --defaults-file=${DIR}/my-client.cnf < ${DIR}/${file}.mysql                                 
done

echo "sciSQL procedures loading in MySQL SUCCESSFUL"

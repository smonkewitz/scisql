cp /home/fjammes/tmp/scisql/lib/libscisql-scisql_0.3.so

/home/fjammes/src/scisql_github/tools/substitute.py ../../scripts/deploy.mysql | /data/fjammes/stack/Linux64/mysql/5.1.65/bin/mysql --defaults-file=/home/fjammes/src/scisql_github/build/c4che/.my.cnf
/home/fjammes/src/scisql_github/tools/substitute.py ../../scripts/demo.mysql | /data/fjammes/stack/Linux64/mysql/5.1.65/bin/mysql --defaults-file=/home/fjammes/src/scisql_github/build/c4che/.my.cnf

# export next variable by reading scisql-build-info.cfg 
# export SCISQL_PREFIX=scisql_
# export SCISQL_VSUFFIX=?

# create next file and export variable
export MYSQL_CNF=my.scisql.cnf

# et lancer tous les tests

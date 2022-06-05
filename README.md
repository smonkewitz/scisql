Science Tools for MySQL
=======================

sciSQL provides science-specific tools and extensions for SQL. Currently,
the project contains user defined functions (UDFs) and stored procedures for
MySQL or MariaDB in the areas of spherical geometry, statistics, and photometry.
It is distributed under the terms of the Apache License version 2.0.

Note that the [docs/index.html](docs/index.html) file in the sciSQL
distribution contains all the instructions below, as well as documentation
for all the sciSQL UDFs and stored procedures.

Installation prerequisites
--------------------------

- [Python 3.7.x or later](http://www.python.org/download/)
- [MySQL server 8.x](https://dev.mysql.com/downloads/mysql/) -or- [MariaDB server 10.x](https://mariadb.com/downloads/)
- [mysqlclient 2.1.x or later](https://github.com/PyMySQL/mysqlclient)
- [Mako 0.4 or later](http://www.makotemplates.org/download.html)

`mysqlclient`, a Python DB API 2.0 implementation for MySQL/MariaDB, is required
in order to run the unit tests and uninstall sciSQL. The Mako templating library is
required only if you wish to rebuild the HTML documentation.

In order to install the UDFs, you need write permission to the MySQL/MariaDB server
plug-in directory as well as a MySQL/MariaDB account with admin priviledges.

Databases reserved for sciSQL use
---------------------------------

The following database names are reserved for use by sciSQL:

| Database name | Description                                                       |
| ------------- | ----------------------------------------------------------------- |
| `scisql`      | Contains sciSQL stored procedures                                 |
| `scisql_test` | Used by sciSQL unit tests                                         |
| `scisql_demo` | Contains sample data that can be used to exercise the sciSQL UDFs |

The `scisql_demo` database is dropped and re-created during installation. If
you are using it for other things, you must migrate its contents to a different
database prior to installing sciSQL. If you do not, YOU WILL LOSE DATA.

Even though the `scisql` and `scisql_test` databases are never automatically
dropped, their use is STRONGLY DISCOURAGED.

Build configuration
--------------------

Run `./configure` from the top-level sciSQL directory. Passing `--help` will
yield a list of configuration options. Here are the ones most likely to require
tweaking if `configure` doesn't work straight out of the box on your system:

| Option             | Description                                                              |
| ------------------ | ------------------------------------------------------------------------ |
| `--mysql-dir`      | Set this to the top-level MySQL/MariaDB server install tree              |
| `--mysql-config`   | Point to `mysql_config` or `mariadb_config` configuration tool           |
| `--mysql-includes` | Point to MySQL/MariaDB headers (`mysql.h` and dependents)                |
| `--scisql-prefix`  | Prefix for all UDF and stored procedure names. The default is "sciscl_". |

If you wish to build/install only the sciSQL client utilities and documentation,
run configure with the `--client-only` option. In this case, a MySQL/MariaDB server or
client install is not required, the only executable generated is `scisql_index`
(a utility which generates HTM indexes for tables of circles or polygons stored
in tab-separated-value format).

Building
--------

sciSQL is built and staged with the usual `make` and `make install` commands.

You may wish to regenerate the HTML documentation if you've chosen a
non-default value for `--scisql-prefix`, as the HTML distributed with release
tar-balls is built under the assumption that `--scisql-prefix="scisql_"`.
To do this, run `make html_docs`.

Deploying sciSQL in a MySQL/MariaDB instance
--------------------------------------------

Assuming you've installed scisql in `$PREFIX`, update your `PATH` and
`PYTHONPATH` as described below:

    export PATH="$PREFIX/bin:$PATH"
    export PYTHONPATH="$PREFIX/python:$PATH"

Check that you have access to a local server instance and run `scisql-deploy.py`. Passing
`--help` will yield a list of configuration options. Here are the ones most likely to require
tweaking:

| Option               | Description                                                              |
| -------------------- | ------------------------------------------------------------------------ |
| `--mysql-user`       | Set this to the name of a MySQL/MariaDB admin user; defaults to `root`.  |
| `--mysql-bin`        | Point to the `mysql` or `mariadb` command-line client                    |
| `--mysql-socket`     | Point to the MySQL/MariaDB server UNIX socket file        |
| `--mysql-plugin-dir` | Point to server plugin directory, typically `/usr/lib/mysql/plugin`      |
| `--verbose`          | Verbosity level; possible value are FATAL, ERROR, WARN, INFO, DEBUG      |

You will be prompted for the MySQL/MariaDB admin user password during configuration.
If you run `scisql-deploy.py` in a script, you can
use standard input, for example via a pipe, for providing this password.
Connection details, including this password, are stored in a temporary directory in a file named
`my-client.cnf` using the MySQL options file format. This temporary file is removed at the end
of the process, unless you set the verbosity level to DEBUG.

The `scisql-deploy.py` command will CREATE the sciSQL UDFs, stored procedures, and
databases (including the scisql_demo database). It will also automatically
run the sciSQL integration tests, which check that sciSQL is correctly deployed.
You can re-run the tests anytime by invoking `scisql-deploy.py` with the `--test` option.

Finally, note that after installation each UDF will be available under two
names: one including a version number and one without. For example, assuming
that `--scisql-prefix="foo_"` and that the sciSQL version number is `1.2.3`,
a hypothetical UDF named "bar" would be available as:

    foo_bar
    foo_bar_1_2_3

Invoking `scisql-deploy.py` with the `--undeploy` option
will drop the versioned sciSQL UDFs and stored
procedures. It will also drop the unversioned UDFs/procedures, but only
if the unversioned UDF/procedure was created by the version of
sciSQL being uninstalled. As a consequence, it is possible to have
multiple versions of sciSQL installed at the same time, and the behavior
of two versions can be compared from within a single MySQL/MariaDB instance.
An unversioned name will resolve to the most recently installed versioned
name.

An undeploy will also drop the scisql_demo database.

Rebuilding sciSQL
-----------------

If you've already installed sciSQL, say using a UDF/procedure name
prefix of "foo_", and then decide you'd like to change the prefix to "bar_",
do the following from the top-level sciSQL directory:

    % scisql-deploy.py ... --undeploy       # Uninstalls the UDFs and stored procedures named foo_*
    % make distclean                        # Removes all build products and configuration files
    % configure --scisql-prefix=bar_ ...    # Reconfigures sciSQL (sets new name prefix)
    % make                                  # Rebuilds sciSQL
    % make install                          # Restages sciSQL
    % scisql-deploy.py ...                  # Redeploys sciSQL

No MySQL/MariaDB restart is required. Note that multiple installations of the same
version of sciSQL with different UDF/procedure name prefixes can coexist on
a single MySQL/MariaDB instance.

MySQL/MariaDB server restarts
-----------------------------

Installing different versions of sciSQL, or multiple copies of the same
version with different UDF/procedure name prefixes, does not require a
server restart.

Only sciSQL developers should need to perform restarts. They are
required when changing the name of a UDF without changing the name of the
shared library installed into the server plugin directory. In this case,
an attempt to install the updated shared library will sometimes result in
MySQL/MariaDB reporting that it cannot find symbol names that are actually
present. This is presumably due to server and/or OS level caching
effects, and restarting the server resolves the problem.

Reporting bugs and getting help
-------------------------------

If you encounter test-case failures, or think you've identified a
bug in the sciSQL code, or would just like to ask a question, please
[submit an issue](https://github.com/smonkewitz/scisql/issues).

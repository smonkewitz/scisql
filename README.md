Science Tools for MySQL
=======================

    sciSQL provides science-specific tools and extensions for SQL. Currently,
the project contains user defined functions (UDFs) and stored procedures for
MySQL in the areas of spherical geometry, statistics, and photometry. It is
distributed under the terms of the Apache License version 2.0.

    Note that the docs/index.html file in the sciSQL distribution contains
all the instructions below, as well as documentation for all the sciSQL UDFs
and stored procedures.


Installation prerequisites:
---------------------------

- Python 2.5.x or later       (http://www.python.org/download/)
- Python future 0.16 or later (http://python-future.org/index.html)
- MySQL server 5.x            (http://dev.mysql.com/downloads/)
- MySQLdb 1.2.x               (http://sourceforge.net/projects/mysql-python/)
- Mako 0.4 or later           (http://www.makotemplates.org/download.html)

MySQLdb, a Python DB API 2.0 implementation for MySQL, is required in order
to run the unit tests and uninstall sciSQL. The Mako templating library is
required only if you wish to rebuild the HTML documentation. The Python
future library is required to allow building this software with both Python
versions 2 and 3.
 
In order to install the UDFs, you need write permission to the MySQL server
plug-in directory as well as a MySQL account with admin priviledges.


Databases reserved for sciSQL use:
----------------------------------

The following database names are reserved for use by sciSQL:

- scisql                  Contains sciSQL stored procedures.
- scisql_test             Used by sciSQL unit tests.
- scisql_demo             Contains sample data that can be used to
                          exercise the sciSQL UDFs.

The scisql_demo databases is dropped and re-created during installation. If you
are using it for other things, you must migrate its contents to a different
database prior to installing sciSQL. If you do not, YOU WILL LOSE DATA.

Even though the scisql and scisql_test databases are never automatically
dropped, their use is STRONGLY DISCOURAGED.


sciSQL configuration:
---------------------

    Run ./configure from the top-level sciSQL directory. Passing --help will
yield a list of configuration options. Here are the ones most likely to require
tweaking:

  --prefix        Set this to the top-level MySQL server install directory
  --mysql-user    Set this to the name of a MySQL admin user
  --mysql-socket  Set this to the name of the MySQL server UNIX socket file
  --scisql-prefix This string will be used as a prefix for all sciSQL UDF
                  and stored procedure names. You can specify an empty
                  string, which will result in unprefixed names. The
                  default is "scisql_".

You will be prompted for the MySQL admin user password during configuration.
Connection details, including this password, are stored in
build/c4che/.my.cnf in the MySQL options file format. This allows various
build steps to connect to MySQL without constantly prompting for a password.

Even though the .my.cnf file permissions are set such that only its creator
is allowed read/write access, for security reasons it is still recommended to
run `make distclean` once sciSQL has been installed. This will remove the
entire build directory and its contents.

If you wish to build/install only the sciSQL client utilities and documentation,
run configure with the --client-only option. In this case, a MySQL server or
client install is not required, the only executable generated is scisql_index
(a utility which generates HTM indexes for tables of circles or polygons stored
in tab-separated-value format), and --prefix can be set to a directory of your
choice.


Building and installation:
--------------------------

    sciSQL is built, installed, and uninstalled with the usual make,
make install, and make uninstall commands.

The install command will CREATE the sciSQL UDFs, stored procedures, and
databases (including the scisql_demo database). It will also automatically
run the sciSQL unit tests. You can re-run the tests anytime with `make test`.

You may wish to regenerate the HTML documentation if you've chosen a 
non-default value for --scisql-prefix, as the HTML distributed with release
tar-balls is built under the assumption that --scisql-prefix="scisql_".
To do this, run `make html_docs`.

Finally, note that after installation each UDF will be available under two
names: one including a version number and one without. For example, assuming
that --scisql-prefix="foo_" and that the sciSQL version number is 1.2.3,
a hypothetical UDF named "bar" would be available as:

    foo_bar
    foo_bar_1_2_3

The uninstall command will drop the versioned sciSQL UDFs and stored
procedures. It will also drop the unversioned UDFs/procedures, but only
if the unversioned UDF/procedure was created by the version of
sciSQL being uninstalled. As a consequence, it is possible to have
multiple versions of sciSQL installed at the same time, and the behavior
of two versions can be compared from within a single MySQL instance.
An unversioned name will resolve to the most recently installed versioned
name.

The uninstall command will also drop the scisql_demo database.


Rebuilding sciSQL:
------------------

    If you've already installed sciSQL, say using a UDF/procedure name
prefix of "foo_", and then decide you'd like to change the prefix to "bar_",
do the following from the top-level sciSQL directory:

    `make uninstall`
        Uninstalls the UDFs and stored procedures named foo_*.
    `make distclean`
        Removes all build products and configuration files
    `configure --scisql-prefix=bar_ ...`
        Reconfigures sciSQL (sets new name prefix)
    `make`
	Rebuilds sciSQL.
    `make install`
        Reinstalls sciSQL.

No MySQL restart is required. Note that multiple installations of the same
version of sciSQL with different UDF/procedure name prefixes can coexist on
a single MySQL instance.


MySQL server restarts:
----------------------

    Installing different versions of sciSQL, or multiple copies of the same
version with different UDF/procedure name prefixes, does not require a MySQL
server restart.

    Only sciSQL developers should need to perform restarts. They are
required when changing the name of a UDF without changing the name of the
shared library installed into the MySQL plugin directory. In this case,
an attempt to install the updated shared library will sometimes result in
MySQL reporting that it cannot find symbol names that are actually
present. This is presumably due to MySQL server and/or OS level caching
effects, and restarting MySQL resolves the problem.


Reporting bugs and getting help:
--------------------------------

    If you encounter test-case failures, or think you've identified a
bug in the sciSQL code, please file a report here:

https://bugs.launchpad.net/scisql/+filebug

    For other help or inquiries, submit your questions here:

https://answers.launchpad.net/scisql/+addquestion


#! /usr/bin/env python
# encoding: utf-8
#
# Copyright (C) 2011 Serge Monkewitz
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Authors:
#     - Serge Monkewitz, IPAC/Caltech
#
# Work on this project has been sponsored by LSST and SLAC/DOE.
#

from __future__ import with_statement
import getpass
import operator
import os
import stat

from waflib import Configure, Logs, Task, TaskGen

__ver = {
    'atleast_version': operator.ge,
    'exact_version': operator.eq,
    'max_version': operator.le,
}

def __parse_version(version):
    return tuple(map(int, version.split('.')))


def options(ctx):
    ctx.add_option('--mysql', type='string', dest='mysql',
                   help='''Path to the mysql command line client (e.g. /usr/local/bin/mysql).
                           Used to CREATE and DROP the scisql UDFs after installation.
                           Defaults to ${PREFIX}/bin/mysql.''')
    ctx.add_option('--mysql-config', type='string', dest='mysql_config',
                   help='''Path to the mysql_config script (e.g. /usr/local/bin/mysql_config).
                           Used to obtain the location of MySQL header files and plugins.''')
    ctx.add_option('--mysql-includes', type='string', dest='mysql_includes',
                   help='''Path to the directory where the MySQL header files are located.
                           Defaults to ${PREFIX}/include/mysql; ignored if --mysql-config is used.''')
    ctx.add_option('--mysql-plugins', type='string', dest='mysql_plugins',
                   help='''Path to the MySQL server plugin directory (UDF installation directory).
                           Defaults to ${PREFIX}/lib/mysql/plugin/; ignored if --mysql-config is used.''')
    ctx.add_option('--mysql-user', type='string', dest='mysql_user', default='root',
                   help='MySQL user name with admin priviledges')
    ctx.add_option('--mysql-socket', type='string', dest='mysql_socket', default='/tmp/mysql.sock',
                   help='UNIX socket file for connecting to MySQL')

@Configure.conf
def check_mysql(self, **kw):
    # Check for the MySQL command line client
    self.start_msg('Checking for mysql command line client')
    mysql = self.options.mysql
    if mysql:
        if not os.path.isfile(mysql) or not os.access(mysql, os.X_OK):
            self.fatal('--mysql does not identify an executable')
        self.end_msg(mysql)
    else:
        mysql = os.path.join(self.env.PREFIX, 'bin', 'mysql')
        if not os.path.isfile(mysql) or not os.access(mysql, os.X_OK):
            self.fatal('${PREFIX}/bin/mysql does not identify an executable')
    self.env.MYSQL = mysql
    self.env.MYSQL_DIR = self.env.PREFIX
    self.end_msg(mysql)
    self.env.MYSQL_USER = self.options.mysql_user
    self.env.MYSQL_SOCKET = self.options.mysql_socket

    # Check for the MySQL config script
    config = self.options.mysql_config
    if config:
        if not os.path.isfile(config) or not os.access(config, os.X_OK):
            self.fatal('--mysql-config does not identify an executable')
        includes = self.cmd_and_log([config, '--include'])
        plugins = self.cmd_and_log([config, '--plugindir'])
    else:
        includes = self.options.mysql_includes or os.path.join(self.env.PREFIX, 'include', 'mysql')
        plugins = self.options.mysql_plugins or os.path.join(self.env.PREFIX, 'lib', 'mysql', 'plugin')

    # Get include directory
    self.start_msg('Checking for mysql include directory')
    if not includes or not os.path.isdir(includes):
        self.fatal('Invalid/missing mysql header directory')
    else:
        self.end_msg(includes)
    self.env.INCLUDES_MYSQL = [includes]

    # Get plugin directory (for UDF installation)
    self.start_msg('Checking for mysql plugins directory')
    if not plugins or not os.path.isdir(plugins):
        self.fatal('Invalid/missing MySQL plugin directory.')
    else:
        self.end_msg(plugins)
    self.env.MYSQL_PLUGIN_DIR = plugins

    # Get the server version
    version = self.check_cc(fragment='''#include "mysql.h"
                                        int main() {
                                            printf(MYSQL_SERVER_VERSION);
                                            return 0;
                                        }''',
                            execute=True,
                            define_ret=True,
                            use='MYSQL',
                            msg='Checking for mysql.h')
    if any(vc in kw for vc in __ver.keys()):
        # Make sure version constraints are satsified
        self.start_msg('Checking MySQL version')
        try:
            mv = __parse_version(version)
        except:
            self.fatal('Invalid MYSQL_SERVER_VERSION %s' % version)
        for constraint in __ver.keys():
            if constraint in kw:
                try:
                    dv = __parse_version(kw[constraint])
                except:
                    self.fatal('Invalid %s value %s' % (constraint, kw[constraint]))
                if not __ver[constraint](mv, dv):
                    self.fatal('MySQL server version %s violates %s=%s' % (version, constraint, kw[constraint]))
        self.end_msg(version)

    # Write MySQL connection parameters to a file to avoid
    # constantly prompting for them
    passwd = getpass.getpass('Enter password for MySQL user %s: ' % self.env.MYSQL_USER)
    self.start_msg('Writing MySQL connection parameters')
    my_cnf = self.path.get_bld().make_node('c4che/.my.cnf').abspath()
    with open(my_cnf, 'wb') as f:
        # avoid fchmod to allow Python 2.5
        os.chmod(my_cnf, stat.S_IRUSR | stat.S_IWUSR)
        f.write('[mysql]\n')
        f.write('user=%s\n' % self.env.MYSQL_USER)
        f.write('socket=%s\n' % self.env.MYSQL_SOCKET)
        if len(passwd) != 0:
            f.write('password=%s\n' % passwd)
    self.env.MYSQL_CNF = my_cnf
    self.end_msg(my_cnf)


# Task generator for running .mysql script files

@TaskGen.extension('.mysql')
def process_mysql(self, node):
    self.create_task('MySqlScript', node, [])

#@Task.always_run
class MySqlScript(Task.Task):
    run_str = '${bld.top_dir}/tools/substitute.py ${SRC} | ${MYSQL} --defaults-file=${MYSQL_CNF}'
    color = 'PINK'
    shell = True
    ext_in = '.mysql'
    reentrant = False
    install_path = False
    after = ['vnum', 'inst']
    vars = ['MYSQL', 'MYSQL_CNF']

MySqlScript = Task.always_run(MySqlScript)


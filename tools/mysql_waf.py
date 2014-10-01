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
import os
import mysqlversion

from waflib import Configure, Logs, Task, TaskGen

def options(ctx):
    ctx.add_option('--mysql-dir', type='string', dest='mysql_dir',
                   help="Path to the mysql install directory. "+
                        "Defaults to ${PREFIX}.")
    ctx.add_option('--mysql-config', type='string', dest='mysql_config',
                   help="Path to the mysql_config script (e.g. /usr/local/bin/mysql_config). "+
                        "Used to obtain the location of MySQL header files.")
    ctx.add_option('--mysql-includes', type='string', dest='mysql_includes',
                   help="Path to the directory where the MySQL header files are located. " +
                        "Defaults to ${mysql-dir}/include/mysql; ignored if --mysql-config is used.")

@Configure.conf
def check_mysql(self):
    self.start_msg('Checking for mysql install')
    mysql_dir = self.options.mysql_dir
    if mysql_dir:
        if not os.path.isdir(mysql_dir) or not os.access(mysql_dir, os.X_OK):
            self.fatal('--mysql-dir does not identify an accessible directory')
        self.end_msg(mysql_dir)
        self.env.MYSQL_DIR = mysql_dir

    # Check for the MySQL config script
    config = self.options.mysql_config
    includes=None
    if config:
        if not os.path.isfile(config) or not os.access(config, os.X_OK):
            self.fatal('--mysql-config does not identify an executable')
        include_flag = self.cmd_and_log([config, '--include'])
        includes = include_flag.strip('\n\r ')[2:]
    elif self.options.mysql_includes:
        includes = self.options.mysql_includes
    elif self.env.MYSQL_DIR:
        includes = os.path.join(self.env.MYSQL_DIR, 'include', 'mysql')

    # Get include directory
    self.start_msg('Checking for mysql include directory')
    if not includes:
        self.fatal('Undefined mysql header directory, use --help option.')
    elif not os.path.isdir(includes):
        self.fatal('Invalid/missing mysql header directory : {0}'.format(includes))
    else:
        self.end_msg(includes)
    self.env.INCLUDES_MYSQL = [includes]

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
    self.start_msg('Checking MySQL version')
    (ok, msg) = mysqlversion.check(version)
    if not ok:
        self.fatal(msg)
    self.end_msg(version)


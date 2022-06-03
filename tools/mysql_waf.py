#! /usr/bin/env python
# encoding: utf-8
#
# Copyright (C) 2011-2022 the SciSQL authors
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

import os
import mysqlversion

from waflib import Configure, Logs, Task, TaskGen


def options(ctx):
    ctx.add_option('--mysql-dir', type='string', dest='mysql_dir',
                   help="Path to the mysql install directory. " +
                        "Defaults to ${PREFIX}.")
    ctx.add_option('--mysql-config', type='string', dest='mysql_config',
                   help="Path to the mysql_config script (e.g. /usr/local/bin/mysql_config). " +
                        "Used to obtain the location of MySQL header files.")
    ctx.add_option('--mysql-includes', type='string', dest='mysql_includes',
                   help="Path to the directory where the MySQL header files are located. " +
                        "Defaults to ${mysql-dir}/include/mysql; ignored if --mysql-config is used.")


@Configure.conf
def check_mysql(self):

    mysql_dir = self.options.mysql_dir
    if mysql_dir:
        self.start_msg('Checking mysql/mariadb install directory')
        if not os.path.isdir(mysql_dir) or not os.access(mysql_dir, os.X_OK):
            self.fatal('--mysql-dir does not identify an accessible directory')
        self.env.MYSQL_DIR = mysql_dir
        self.end_msg(mysql_dir)

    self.start_msg('Checking mysql/mariadb config tool')
    config = self.options.mysql_config
    if config:
        if not os.path.isfile(config) or not os.access(config, os.X_OK):
            self.fatal('--mysql-config does not identify an executable')
    elif self.env.MYSQL_DIR:
        config_prefix = os.path.join(self.env.MYSQL_DIR, 'bin')
        config_mariadb = os.path.join(config_prefix, 'mariadb_config')
        config_mysql = os.path.join(config_prefix, 'mysql_config')
        if self.exec_command(config_mariadb) == 0:
            config = config_mariadb
        elif self.exec_command(config_mysql) == 0:
            config = config_mysql
    else:
        try:
            config = self.cmd_and_log(['which', 'mariadb_config']).strip()
        except:
            try:
                config = self.cmd_and_log(['which', 'mysql_config']).strip()
            except:
                config = None
    self.end_msg(config)

    self.start_msg('Checking mysql/mariadb include directories')
    includes = None
    if self.options.mysql_includes:
        includes = self.options.mysql_includes.split(',')
    elif config:
        include_flag = self.cmd_and_log([config, '--include']).strip()
        includes = include_flag.replace('-I','').split()
    elif self.env.MYSQL_DIR:
        includes = os.path.join(self.env.MYSQL_DIR, 'include', 'mysql')
    if not includes:
        self.fatal('Undefined mysql/mariadb include directories, use --help option.')
    elif not all(os.path.isdir(i) for i in includes):
        self.fatal('Invalid/missing mysql/mariadb include directory : {0}'.format(includes))
    self.env.INCLUDES_MYSQL = includes
    self.end_msg(includes)

    self.start_msg('Checking mysql/mariadb version')
    version = self.check_cc(fragment='''#include "mysql.h"
                                        int main() {
                                            printf(MYSQL_SERVER_VERSION);
                                            return 0;
                                        }''',
                            execute=True,
                            define_ret=True,
                            use='MYSQL',
                            msg='Checking for mysql.h')
    (ok, msg) = mysqlversion.check(version)
    if not ok:
        self.fatal(msg)
    self.end_msg(version)

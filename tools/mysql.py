#! /usr/bin/env python
# encoding: utf-8
import operator
import os, os.path

from waflib import Logs
from waflib.Configure import conf

__ver = {
    'atleast_version': operator.ge,
    'exact_version': operator.eq,
    'max_version': operator.le,
}

def __parse_version(version):
    return tuple(map(int, version.split('.')))


def options(ctx):
    ctx.add_option('--mysql-config', type='string', dest='mysql_config',
                   help='''Path to the mysql_config script (e.g. /usr/local/bin/mysql_config).
                           Used to obtain the location of MySQL header files and plugins.''')
    ctx.add_option('--mysql-includes', type='string', dest='mysql_includes',
                   help='''Path to the directory where the MySQL header files are located.
                           Defaults to ${PREFIX}/include/mysql; ignored if --mysql-config is used.''')
    ctx.add_option('--mysql-plugins', type='string', dest='mysql_plugins',
                   help='''Path to the MySQL server plugin directory (UDF installation directory).
                           Defaults to ${PREFIX}/lib/mysql/plugin/; ignored if --mysql-config is used.''')

@conf
def check_mysql(self, **kw):
    config = self.options.mysql_config
    if config:
        if not os.path.isfile(config) or not os.access(config, os.X_OK):
            self.fatal('--mysql-config does not identify an executable file')
        includes = self.cmd_and_log([config, '--includes'])
        plugins = self.cmd_and_log([config, '--plugindir'])
    else:
        includes = self.options.mysql_includes or os.path.join(self.env.PREFIX, 'include', 'mysql')
        plugins = self.options.mysql_plugins or os.path.join(self.env.PREFIX, 'lib', 'mysql', 'plugin')
    # Make sure we have a mysql includes directory
    self.start_msg('Checking for mysql include directory')
    if not includes or not os.path.isdir(includes):
        self.fatal('Invalid/missing mysql header directory')
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
    if any(vc in kw for vc in __ver.keys()):
        # Make sure desired version constraints are satsified
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

    # Get plugin directory (required for UDF installation)
    if not plugins or not os.path.isdir(plugins):
        Logs.pprint('YELLOW', 'Invalid/missing MySQL plugin directory. You will not ' +
                    'be able to install, load, or test the scisql UDFs.')
    else:
        self.env.MYSQL_PLUGIN_DIR = plugins


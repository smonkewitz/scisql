#! /usr/bin/env python
# encoding: utf-8
#
# Copyright (C) 2011 Serge Monkewitz
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License v3 as published
# by the Free Software Foundation, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
# A copy of the LGPLv3 is available at <http://www.gnu.org/licenses/>.
#
# Authors:
#     - Serge Monkewitz, IPAC/Caltech
#
# Work on this project has been sponsored by LSST and SLAC/DOE.
#

from __future__ import with_statement
import os
import sys
import traceback

from waflib import Build, Logs, Utils

APPNAME = 'scisql'
VERSION = '0.1'

top = '.'
out = 'build'


def options(ctx):
    ctx.load('compiler_c')
    ctx.load('mysql', tooldir='tools')

def configure(ctx):
    ctx.load('compiler_c')
    ctx.load('mysql', tooldir='tools')
    ctx.check_mysql(atleast_version='5')
    ctx.env['CFLAGS'] = ['-Wall',
                         '-Wextra',
                         '-O3'
                        ]

    # Test for __attribute__ support
    ctx.check_cc(fragment='''__attribute__ ((visibility("default"))) int foo() { return 0; }
                             __attribute__ ((visibility("hidden"))) int bar() { return 0; }
                             int main() { return foo() + bar(); }''',
                 define_name='HAVE_ATTRIBUTE_VISIBILITY',
                 execute=True,
                 msg='Checking for __attribute__ ((visibility()))')
    ctx.check_cc(fragment='''void foo(int x __attribute__ ((unused))) { }
                             __attribute__ ((unused)) void bar() { }
                             int main() { return 0; }''',
                 define_name='HAVE_ATTRIBUTE_UNUSED',
                 msg='Checking for __attribute__ ((unused))')
    ctx.check_cc(fragment='''typedef struct { double a; double b; } test __attribute__ ((aligned(16)));
                             int main() { return 0; }''',
                 define_name='HAVE_ATTRIBUTE_ALIGNED',
                 msg='Checking for __attribute__ ((aligned()))')

    # Check for libm
    ctx.check_cc(lib='m', uselib_store='M')

    # Add scisql version to configuration header
    ctx.define(APPNAME.upper() + '_VERSION_STRING', VERSION)
    ctx.define(APPNAME.upper() + '_VERSION_STRING_LENGTH', len(VERSION))
    versions = map(int, VERSION.split('.') + [0, 0])
    for v, n in zip(versions, ('MAJOR', 'MINOR', 'PATCH')):
        ctx.define(APPNAME.upper() + '_VERSION_' + n, v)
    ctx.define(APPNAME.upper() + '_VERSION_NUM',
               versions[0]*100**2 + versions[1]*100 + versions[2])
    ctx.write_config_header('src/config.h')


def build(ctx):
    # UDF shared library
    ctx.shlib(
        source=ctx.path.ant_glob('src/*.c') +
               ctx.path.ant_glob('src/udfs/*.c'),
        includes='src',
        target='scisql',
        name='scisql',
        use='MYSQL M',
        install_path=ctx.env.MYSQL_PLUGIN_DIR
    )
    # C test cases
    ctx.program(
        source='test/testSelect.c src/select.c',
        includes='src',
        target='test/testSelect',
        install_path=False,
        use='M'
    )
    if ctx.cmd == 'install':
        ctx.add_post_fun(create_post)
        ctx.add_post_fun(test)
    elif ctx.cmd == 'uninstall':
        drop(ctx)


def create_post(ctx):
    """Run create_udfs script in a separate build context, from a post-build function.
    This ensures that the scisql shared library has already been installed.
    """
    dir = os.path.join(ctx.path.get_bld().abspath(), '.create_udfs')
    bld = Build.BuildContext(top_dir=ctx.top_dir, run_dir=ctx.run_dir, out_dir=dir)
    bld.init_dirs()
    bld.env = ctx.env
    bld(source='scripts/create_udfs.mysql')
    bld.compile()


class DropContext(Build.BuildContext):
    cmd = 'drop'
    fun = 'drop'

def drop(ctx):
    ctx(source='scripts/drop_udfs.mysql')


class TestContext(Build.BuildContext):
    cmd = 'test'
    fun = 'test'

class Tests(object):
    def __init__(self):
        self.unit_tests = []

    def utest(self, **kw):
        nodes = kw.get('source', [])
        if not isinstance(nodes, list):
            self.unit_tests.append(nodes)
        else:
            self.unit_tests.extend(nodes)

    def run(self, ctx):
        nok, nfail, nexcept = (0, 0, 0)
        for utest in self.unit_tests:
            msg = 'Running %s' % utest
            msg += ' ' * max(0, 40 - len(msg))
            Logs.pprint('CYAN', msg, sep=': ')
            out = utest.change_ext('.log')
            env = os.environ.copy()
            env['MYSQL_CNF'] = ctx.env['MYSQL_CNF']
            with open(out.abspath(), 'wb') as f:
                try:
                    proc = Utils.subprocess.Popen([utest.abspath()],
                                                  shell=False, env=env, stderr=f, stdout=f)
                    proc.communicate()
                except:
                    nexcept += 1
                    ex_type, ex_val, _ = sys.exc_info()
                    msg = traceback.format_exception_only(ex_type, ex_val)[-1].strip()
                    Logs.pprint('RED', msg)
                else:
                    if proc.returncode != 0:
                        nfail += 1
                        Logs.pprint('YELLOW', 'FAIL [see %s]' % out.abspath())
                    else:
                        nok += 1
                        Logs.pprint('CYAN', 'OK')
        if nfail == 0 and nexcept == 0:
            Logs.pprint('CYAN', '\nAll %d tests passed!\n' % nok)
        else:
            Logs.pprint('YELLOW', '\n%d tests passed, %d failed, and %d failed to run\n' %
                        (nok, nfail, nexcept))

def test(ctx):
    tests = Tests()
    tests.utest(source=ctx.path.get_bld().make_node('test/testSelect'))
    tests.utest(source=ctx.path.ant_glob('test/test*.py'))
    tests.run(ctx)


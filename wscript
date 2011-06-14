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

_have_mako = True
try:
    import mako.template
    import mako.lookup
except:
    _have_mako = False


APPNAME = 'scisql'
VERSION = '0.1'

top = '.'
out = 'build'


def options(ctx):
    ctx.add_option('--client-only', dest='client_only', action='store_true',
                   default=False, help='Build client utilities only')
    ctx.add_option('--scisql-prefix', dest='scisql_prefix', default='scisql_',
                   help='UDF/stored procedure name prefix (defaulting to %default). ' +
                        'An empty string means: do not prefix.')
    ctx.load('compiler_c')
    ctx.load('mysql', tooldir='tools')

def configure(ctx):
    ctx.env.SCISQL_CLIENT_ONLY = ctx.options.client_only
    ctx.env.SCISQL_VERSION = VERSION
    ctx.load('compiler_c')
    if not ctx.options.client_only:
        ctx.load('mysql', tooldir='tools')
        ctx.check_mysql(atleast_version='5')
        ctx.define('SCISQL_PREFIX', ctx.options.scisql_prefix, quote=False)
        ctx.env.SCISQL_PREFIX = ctx.options.scisql_prefix
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
    versions = map(int, VERSION.split('.'))
    if len(versions) < 3:
        versions.extend([0]*(3 - len(versions)))
    for v, n in zip(versions, ('MAJOR', 'MINOR', 'PATCH')):
        ctx.define(APPNAME.upper() + '_VERSION_' + n, v)
    ctx.define(APPNAME.upper() + '_VERSION_NUM',
               versions[0]*100**2 + versions[1]*100 + versions[2])
    ctx.env.SCISQL_VSUFFIX = '_' + '_'.join(map(str, versions))
    ctx.define('SCISQL_VSUFFIX', ctx.env.SCISQL_VSUFFIX, quote=False)
    ctx.write_config_header('src/config.h')

    # Create run-time environment for tasks
    if not ctx.options.client_only:
        env = os.environ.copy()    
        env['MYSQL'] = ctx.env.MYSQL
        env['MYSQL_CNF'] = ctx.env.MYSQL_CNF
        env['MYSQL_DIR'] = ctx.env.MYSQL_DIR
        env['SCISQL_PREFIX'] = ctx.env.SCISQL_PREFIX
        env['SCISQL_VERSION'] = VERSION
        env['SCISQL_VSUFFIX'] = ctx.env.SCISQL_VSUFFIX
        ctx.env.env = env


def build(ctx):
    # UDF shared library
    if not ctx.env.SCISQL_CLIENT_ONLY:
        ctx.shlib(
            source=ctx.path.ant_glob('src/*.c') +
                   ctx.path.ant_glob('src/udfs/*.c'),
            includes='src',
            target='scisql-' + ctx.env.SCISQL_PREFIX + VERSION,
            name='scisql',
            use='MYSQL M',
            install_path=ctx.env.MYSQL_PLUGIN_DIR
        )
    # Off-line spatial indexing tool
    ctx.program(
        source='src/util/index.c src/geometry.c src/htm.c',
        includes='src',
        target='scisql_index',
        install_path=os.path.join(ctx.env.PREFIX, 'bin'),
        use='M'
    )
    # C test cases
    ctx.program(
        source='test/testSelect.c src/select.c',
        includes='src',
        target='test/testSelect',
        install_path=False,
        use='M'
    )
    ctx.program(
        source='test/testHtm.c src/geometry.c src/htm.c',
        includes='src',
        target='test/testHtm',
        install_path=False,
        use='M'
    )
    if ctx.env.SCISQL_CLIENT_ONLY:
        doc_dir = ctx.path.find_dir('doc')
        ctx.install_files('${PREFIX}/doc', doc_dir.ant_glob('**/*'),
                          cwd=doc_dir, relative_trick=True)
    if ctx.cmd == 'install' and not ctx.env.SCISQL_CLIENT_ONLY:
        ctx.add_post_fun(create_post)
        ctx.add_post_fun(test)
    elif ctx.cmd == 'uninstall' and not ctx.env.SCISQL_CLIENT_ONLY:
        ctx(rule='${SRC}',
            source='scripts/uninstall.py',
            always=True)


def create_post(ctx):
    """Run create_udfs script in a separate build context, from a post-build function.
    This ensures that the scisql shared library has already been installed.
    """
    dir = os.path.join(ctx.path.get_bld().abspath(), '.mysql')
    bld = Build.BuildContext(top_dir=ctx.top_dir, run_dir=ctx.run_dir, out_dir=dir)
    bld.init_dirs()
    bld.env = ctx.env
    t1 = bld(source='scripts/install.mysql')
    t2 = bld(source='scripts/demo.mysql')
    t1.post()
    t2.post()
    t2.tasks[0].set_run_after(t1.tasks[0])
    bld.compile()


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
            with open(out.abspath(), 'wb') as f:
                try:
                    proc = Utils.subprocess.Popen([utest.abspath()],
                                                  shell=False, env=ctx.env.env or None,
                                                  stderr=f, stdout=f)
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
            ctx.fatal('One or more sciSQL unit tests failed')

def test(ctx):
    tests = Tests()
    tests.utest(source=ctx.path.get_bld().make_node('test/testHtm'))
    tests.utest(source=ctx.path.get_bld().make_node('test/testSelect'))
    if not ctx.env.SCISQL_CLIENT_ONLY:
        tests.utest(source=ctx.path.ant_glob('test/test*.py'))
        tests.utest(source=ctx.path.make_node('tools/docs.py'))
    tests.run(ctx)


class HtmlDocsContext(Build.BuildContext):
    cmd = 'html_docs'
    fun = 'html_docs'

def html_docs(ctx):
    if not _have_mako:
        ctx.fatal('You must install mako 0.4.x to generate HTML documentation')
    ctx(rule='${SRC} html_docs',
        source='tools/docs.py',
        always=True)


class LsstDocsContext(Build.BuildContext):
    cmd = 'lsst_docs'
    fun = 'lsst_docs'

def lsst_docs(ctx):
    if not _have_mako:
        ctx.fatal('You must install mako 0.4.x to generate LSST documentation')
    ctx(rule='${SRC} lsst_docs',
        source='tools/docs.py',
        always=True)


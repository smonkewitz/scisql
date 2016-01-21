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
import stat
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
VERSION = '0.3'

top = '.'
out = 'build'

BUILD_CONST_MODULE='const.py'

def options(ctx):
    ctx.add_option('--client-only', dest='client_only', action='store_true',
                   default=False, help='Build client utilities only')
    ctx.add_option('--scisql-prefix', dest='scisql_prefix', default='scisql_',
                   help='UDF/stored procedure name prefix (defaulting to %default). ' +
                        'An empty string means: do not prefix.')
    ctx.load('compiler_c')
    ctx.load('mysql_waf', tooldir='tools')

def configure(ctx):
    ctx.env.SCISQL_CLIENT_ONLY = ctx.options.client_only
    ctx.env.SCISQL_VERSION = VERSION
    ctx.load('compiler_c')
    if not ctx.options.client_only:
        ctx.load('mysql_waf', tooldir='tools')
        ctx.check_mysql()
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
                 mandatory=False,
                 msg='Checking for __attribute__ ((visibility()))')
    ctx.check_cc(fragment='''void foo(int x __attribute__ ((unused))) { }
                             __attribute__ ((unused)) void bar() { }
                             int main() { return 0; }''',
                 define_name='HAVE_ATTRIBUTE_UNUSED',
                 mandatory=False,
                 msg='Checking for __attribute__ ((unused))')
    ctx.check_cc(fragment='''typedef struct { double a; double b; } test __attribute__ ((aligned(16)));
                             int main() { return 0; }''',
                 define_name='HAVE_ATTRIBUTE_ALIGNED',
                 mandatory=False,
                 msg='Checking for __attribute__ ((aligned()))')
    # Check endianness of platform
    ctx.check_cc(fragment='''union { int val; unsigned char bytes[sizeof(int)]; } u;
                             int main() {
                                 u.val = 0x0201;
                                 return u.bytes[0] != 0x01;
                             }''',
                 define_name='IS_LITTLE_ENDIAN',
                 execute=True,
                 mandatory=False,
                 okmsg='ok',
                 msg='Checking byte order')

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
    ctx.env.SCISQL_LIBNAME = ctx.env['cshlib_PATTERN'] % ('scisql-' + ctx.env.SCISQL_PREFIX + VERSION)
    ctx.define('SCISQL_VSUFFIX', ctx.env.SCISQL_VSUFFIX, quote=False)
    ctx.write_config_header('src/config.h')

    # Create run-time environment for tasks
    if not ctx.options.client_only:
        env = os.environ.copy()
        env['SCISQL_PREFIX'] = ctx.env.SCISQL_PREFIX
        env['SCISQL_VERSION'] = VERSION
        env['SCISQL_VSUFFIX'] = ctx.env.SCISQL_VSUFFIX
        env['SCISQL_LIBNAME'] = ctx.env.SCISQL_LIBNAME
        ctx.env.env = env
        ctx.start_msg('Writing build parameters')
        dest = ctx.bldnode.make_node(BUILD_CONST_MODULE)
        dest.parent.mkdir()
        dest.write(
            "SCISQL_PREFIX = \"{0}\"\n".format(ctx.env.SCISQL_PREFIX) +
            "SCISQL_VERSION = \"{0}\"\n".format(ctx.env.SCISQL_VERSION) +
            "SCISQL_VSUFFIX = \"{0}\"\n".format(ctx.env.SCISQL_VSUFFIX) +
            "SCISQL_LIBNAME = \"{0}\"\n".format(ctx.env.SCISQL_LIBNAME)
        )
        ctx.env.append_value('cfg_files', dest.abspath())
        ctx.end_msg(BUILD_CONST_MODULE)

def build(ctx):
    # UDF shared library
    if not ctx.env.SCISQL_CLIENT_ONLY:
        libname='scisql-' + ctx.env.SCISQL_PREFIX + VERSION
        ctx.shlib(
            source=ctx.path.ant_glob('src/*.c') +
                   ctx.path.ant_glob('src/udfs/*.c'),
            includes='src',
            target=libname,
            name='scisql',
            use='MYSQL M',
            install_path=os.path.join(ctx.env.PREFIX, 'lib')
        )

    # Off-line spatial indexing tool
    ctx.program(
        source='src/util/index.c src/geometry.c src/htm.c',
        includes='src',
        target='scisql_index',
        install_path=os.path.join(ctx.env.PREFIX, 'bin'),
        use='M'
    )
    # C test cases, executed in build process, against shared library
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
    # doc directory
    doc_dir = ctx.path.find_dir('doc')
    ctx.install_files('${PREFIX}/doc', doc_dir.ant_glob('**/*'),
                          cwd=doc_dir, relative_trick=True)
    # bin directory
    bin_dir = ctx.path.find_dir('bin')
    ctx.install_files('${PREFIX}/bin', bin_dir.ant_glob('**/*.py'),
                          chmod=Utils.O755, cwd=bin_dir, relative_trick=True)

    if not ctx.env.SCISQL_CLIENT_ONLY:
        # install build configuration module whose parameters will be used by
        # deployment script
        ctx.install_files('${PREFIX}/python/scisql', BUILD_CONST_MODULE)

        # python modules
        python_dir = ctx.path.find_dir('python')
        ctx.install_files('${PREFIX}/python', python_dir.ant_glob('**/*.py'),
                              cwd=python_dir, relative_trick=True)
        # tools directory
        tool_dir = ctx.path.find_dir('tools')
        ctx.install_files('${PREFIX}/tools', tool_dir.ant_glob('**/*'),
                              cwd=tool_dir, relative_trick=True)
        # template directory
        template_dir = ctx.path.find_dir('templates')
        ctx.install_files('${PREFIX}/templates', template_dir.ant_glob('**/*'),
                              cwd=template_dir, relative_trick=True)
        # python tests
        test_dir = ctx.path.find_dir('test')
        ctx.install_files('${PREFIX}/test', test_dir.ant_glob('**/*.py'),
                              cwd=test_dir, relative_trick=True)
    if ctx.cmd == 'build':
        ctx.add_post_fun(test)


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


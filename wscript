#! /usr/bin/env python
# encoding: utf-8

from waflib import Build, Options, Scripting, Task

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
    install_path = ctx.env.MYSQL_PLUGIN_DIR
    node = ctx.shlib(
        source=ctx.path.ant_glob('src/*.c'),
        includes=['src'],
        target='scisql',
        name='scisql',
        use=['MYSQL', 'M'],
        install_path=install_path
    )
    if ctx.cmd in ('install', 'create'):
        ctx.add_post_fun(create_post)
    elif ctx.cmd == 'uninstall':
        drop(ctx)


class CreateContext(Build.InstallContext):
    cmd = 'create'
    fun = 'build'

def create_post(ctx):
    """Run create_udfs script in a separate build context, from a post-build function.
    This ensures that the scisql shared library has already been installed.
    """
    bld = Build.BuildContext(top_dir=ctx.top_dir, run_dir=ctx.run_dir)
    bld.init_dirs()
    bld.env = ctx.env
    bld(source='scripts/create_udfs.mysql')
    bld.compile()

class DropContext(Build.BuildContext):
    cmd = 'drop'
    fun = 'drop'

def drop(ctx):
    ctx(source='scripts/drop_udfs.mysql')
    


def test(ctx):
    # TODO
    ctx.fatal('Not implemented yet')


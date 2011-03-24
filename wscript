#! /usr/bin/env python
# encoding: utf-8

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
                         '-Wextra'
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
 
    # Add scisql version to configuration header
    ctx.define(APPNAME.upper() + '_VERSION_STRING', VERSION)
    ctx.define(APPNAME.upper() + '_VERSION_STRING_LENGTH', len(VERSION))
    versions = map(int, VERSION.split('.') + [0, 0])
    for v, n in zip(versions, ('MAJOR', 'MINOR', 'PATCH')):
        ctx.define(APPNAME.upper() + '_VERSION_' + n, v)
    ctx.define(APPNAME.upper() + '_VERSION_NUM',
               versions[0]*100**2 + versions[1]*100 + versions[2])
    ctx.write_config_header('src/config.h')

def post(ctx):
    if ctx.cmd in ('install', 'uninstall', 'create', 'drop', 'test') and not ctx.env.MYSQL_PLUGIN_DIR:
        ctx.fatal('Invalid or missing MySQL plugin installation dir: ' +
                  'cannot install, uninstall, create, drop or test UDFs')

def build(ctx):
    ctx.add_post_fun(post)
    install_path = ctx.env.MYSQL_PLUGIN_DIR
    ctx.shlib(
        source=ctx.path.ant_glob('src/*.c'),
        includes=['src'],
        target='scisql',
        name='scisql',
        use='MYSQL',
        install_path=install_path
    )

def create(ctx):
    # TODO
    ctx.fatal('Not implemented yet')

def drop(ctx):
    # TODO
    ctx.fatal('Not implemented yet')

def test(ctx):
    # TODO
    ctx.fatal('Not implemented yet')


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

def post(ctx):
    if ctx.cmd in ('install', 'uninstall', 'test') and not ctx.env.MYSQL_PLUGIN_DIR:
        ctx.fatal('Invalid or missing MySQL plugin installation dir: cannot install or test')

def build(ctx):
    ctx.add_post_fun(post)
    install_path = ctx.env.MYSQL_PLUGIN_DIR
    ctx.shlib(
        source = ctx.path.ant_glob('src/*.c'),
        target = 'scisql',
        name   = 'scisql',
        use    = 'MYSQL',
        install_path=install_path
    )

def test(ctx):
    # TODO
    pass


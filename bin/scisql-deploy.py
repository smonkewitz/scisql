#!/usr/bin/env python

import argparse
import ConfigParser
import fileinput
import getpass
import glob
import logging
import os
import shutil
import stat
from subprocess import check_output
import sys
import tempfile
import time

STEP_LIST = ['deploy', 'test']
STEP_DOC = dict(
    zip(STEP_LIST,
        [
        """Deploy sciSQL""",
        """Launch tests on sciSQL plugin install"""
        ]
    )
)

DEPLOY = STEP_LIST[0]
TEST = STEP_LIST[0]

def user_yes_no_query(question):
    sys.stdout.write('\n%s [y/n]\n' % question)
    while True:
        try:
            return strtobool(raw_input().lower())
        except ValueError:
            sys.stdout.write('Please respond with \'y\' or \'n\'.\n')

def parse_args():

    parser = argparse.ArgumentParser(
            description='''sciSQL deployment tool. Install sciSQL shared
library in MySQL plugin directory and create UDF''',
            formatter_class=argparse.ArgumentDefaultsHelpFormatter
            )

    # Defining option of each configuration step
    for step_name in STEP_LIST:
        parser.add_argument(
            "-{0}".format(step_name[0]),
            "--{0}".format(step_name),
            dest="step_list",
            action='append_const',
            const=step_name,
            help=STEP_DOC[step_name]
            )

    # Logging management
    verbose_dict = {
        'DEBUG'     : logging.DEBUG,
        'INFO'      : logging.INFO,
        'WARNING'   : logging.WARNING,
        'ERROR'     : logging.ERROR,
        'FATAL'     : logging.FATAL,
    }
    verbose_arg_values = verbose_dict.keys()
    parser.add_argument("-v", "--verbose-level", dest="verbose_str",
            choices=verbose_arg_values,
            default='INFO',
            help="verbosity level"
            )

    # forcing options which may ask user confirmation
    parser.add_argument("-f", "--force", dest="force", action='store_true',
            default=False,
            help="forcing removal of existing execution data"
            )

    parser.add_argument("-m", "--mysql-dir", dest="mysql_dir",
            default=os.getenv("MYSQL_DIR"),
            help="""Path to the mysql install directory.
		    Default to MYSQL_DIR environement variable value if not empty"""
            )

    parser.add_argument("-b", "--mysql-bin", dest="mysql_bin",
            default="{mysql_dir}/bin/mysql",
            help="""Path to the mysql command line client (e.g. /usr/local/bin/mysql).
Used to CREATE and DROP the scisql UDFs after installation."""
            )

    parser.add_argument("-P", "--mysql-plugin-dir", dest="mysql_plugin_dir",
            default="{mysql_dir}/lib/mysql/plugin",
            help="full path to MySQL plugin directory"
            )

    parser.add_argument("-S", "--mysql-socket", dest="mysql_socket",
            default=None,
            help="UNIX socket file for connecting to MySQL"
            )

    parser.add_argument("-u", "--mysql-user", dest="mysql_user",
            default='root',
            help="'MySQL user name with admin privileges"
            )

    args = parser.parse_args()

    # password MUST NOT be displayed by ps command
    if sys.stdin.isatty():
        #args.mysql_password = getpass.getpass('Enter MySQL password: ')
        args.mysql_password = "test"
    else:
        print("Reading MySQL password using stdin")
        args.mysql_password = sys.stdin.readline().rstrip()

    if args.step_list is None:
        args.step_list = STEP_LIST

    args.verbose_level = verbose_dict[args.verbose_str]
    return args

def check_global_args(args):
    # replace default values if needed
    args.mysql_bin = args.mysql_bin.format(mysql_dir=args.mysql_dir)
    args.mysql_plugin_dir = args.mysql_plugin_dir.format(mysql_dir=args.mysql_dir)

    logging.info('Checking for mysql command line client')
    if not os.path.isfile(args.mysql_bin) or not os.access(args.mysql_bin, os.X_OK):
        logging.fatal('{0} does not  identify an executable. Use --mysql-dir or --mysql options.'
            .format(args.mysql_bin)
        )
	exit(1)

    logging.info('Checking for mysql socket')
    if args.mysql_socket is None:
        logging.fatal('Missing MySQL socket. Use --mysql-socket options.')
	exit(1)
    else:
        mode = os.stat(args.mysql_socket).st_mode
        is_socket = stat.S_ISSOCK(mode)
	if not is_socket:
            logging.fatal('Invalid MySQL socket. Use --mysql-socket options.'
                .format(args.mysql_socket)
            )
	    exit(1)



def my_cnf(user, password, socket):
    temp = tempfile.NamedTemporaryFile(
            suffix='_my.cnf',
            prefix='scisql_',
            dir='/tmp',
           )
    lines=[
        '[mysql]\n',
        'user={0}\n'.format(user),
        'password={0}\n'.format(password),
        'socket={0}\n'.format(socket)
    ]
    temp.writelines(lines)
    temp.seek(0)
    return temp

def main():

    args = parse_args()

    logging.basicConfig(format='%(levelname)s: %(message)s', level=args.verbose_level)
    logging.info("sciSQL deployment tool\n"+
                 "============================"
    )

    check_global_args(args)


    scisql_dir = os.path.abspath(
                    os.path.join(
                        os.path.dirname(os.path.realpath(__file__)),
                        "..")
                )


    my_cnf_tmpfile = my_cnf(args.mysql_user, args.mysql_password, args.mysql_socket)
    logging.debug("Writing MySQL credentials in {0}".format(my_cnf_tmpfile.name))

    if DEPLOY in args.step_list:

	logging.info("Deploying sciSQL")

	logging.info('Checking for mysql plugins directory')
        if not args.mysql_plugin_dir or not os.path.isdir(args.mysql_plugin_dir):
            logging.fatal('Invalid/missing MySQL plugin directory. Use --mysql-dir or --mysql-plugin-dir options.')
	    exit(1)

        logging.info("Deploying sciSQL shared library in {0}"
		.format(args.mysql_plugin_dir)
        )
	scisql_lib_dir = os.path.join(scisql_dir,"lib")
	libs = glob.glob(scisql_lib_dir+os.path.sep+"*.so")
	for lib in libs:
	    shutil.copy(lib, args.mysql_plugin_dir)

if __name__ == '__main__':
    main()

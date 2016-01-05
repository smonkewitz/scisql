#!/usr/bin/env python

import argparse
import getpass
import logging
import os
import shutil
import stat
from scisql import configure
from scisql import const
from scisql import utils
import sys
import tempfile

def parse_args():

    parser = argparse.ArgumentParser(
            description='''sciSQL deployment tool. Install sciSQL plugin in a
MariaDB/MySQL running instance :\n
- install shared library in MariaDB/MySQL plugin directory\n
- install UDF in MariaDB/MySQL database''',
)

    # Defining option of each configuration step
    for step_name in configure.STEP_LIST:
        parser.add_argument(
            "-{0}".format(step_name[0]),
            "--{0}".format(step_name),
            dest="step_list",
            action='append_const',
            const=step_name,
            help=configure.STEP_DOC[step_name]
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
            help="verbosity level (default: %(default)s)"
            )

    # forcing options which may ask user confirmation
    parser.add_argument("-f", "--force", dest="force", action='store_true',
            default=False,
            help="forcing removal of existing execution data (default: %(default)s)"
            )

    parser.add_argument("-T", "--tmp-dir", dest="tmp_dir",
            default="/tmp",
            help="""Path to directory where deployment temporary data will be stored."""
            )

    parser.add_argument("-m", "--mysql-dir", dest="mysql_dir",
            default=os.getenv("MARIADB_DIR"),
            help="""Path to the MariaDB/MySQL install directory.
                    Default to MARIADB_DIR environement variable value if not empty (default: %(default)s)"""
            )

    parser.add_argument("-b", "--mysql-bin", dest="mysql_bin",
            default="{mysql_dir}/bin/mysql",
            help="""Path to the mysql command line client (e.g. /usr/local/bin/mysql).
Used to CREATE and DROP the scisql UDFs after installation. (default: %(default)s)"""
            )

    parser.add_argument("-P", "--mysql-plugin-dir", dest="mysql_plugin_dir",
            default="{mysql_dir}/lib/plugin",
            help="full path to MariaDB/MySQL plugin directory (default: %(default)s)"
            )

    parser.add_argument("-S", "--mysql-socket", dest="mysql_socket",
            default=None,
            help="UNIX socket file for connecting to MySQL"
            )

    parser.add_argument("-U", "--mysql-user", dest="mysql_user",
            default='root',
            help="MySQL user name with admin privileges (default: %(default)s)"
            )

    args = parser.parse_args()

    # password MUST NOT be displayed by ps command
    if sys.stdin.isatty():
        args.mysql_password = getpass.getpass('Enter MySQL/MariaDB password: ')
    else:
        print("Reading MySQL/MariaDB password using standard input")
        args.mysql_password = sys.stdin.readline().rstrip()

    if args.step_list is None:
        args.step_list = configure.DEFAULT_STEP_LIST

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
        sys.exit(1)

    logging.info('Checking for mysql socket')
    if args.mysql_socket is None:
        logging.fatal('Missing MySQL socket. Use --mysql-socket options.')
        sys.exit(1)
    else:
        mode = os.stat(args.mysql_socket).st_mode
        is_socket = stat.S_ISSOCK(mode)
        if not is_socket:
            logging.fatal('Invalid MySQL socket. Use --mysql-socket options.'
                .format(args.mysql_socket)
            )
            sys.exit(1)

def main():

    scisql_dir = os.path.abspath(
        os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "..")
    )

    args = parse_args()

    logging.basicConfig(format='%(levelname)s: %(message)s', level=args.verbose_level)
    logging.info("sciSQL deployment tool\n"+
                 "============================"
    )

    check_global_args(args)

    scisql_base = os.path.abspath(
                    os.path.join(
                        os.path.dirname(os.path.realpath(__file__)),
                        "..")
                )

    configure.init_config(
        scisql_base,
        args.mysql_bin,
        args.mysql_user,
        args.mysql_password,
        args.mysql_socket
    )

    try:
        tmp_dir = tempfile.mkdtemp(suffix='-scisql', dir=args.tmp_dir)

        scisql_template_dir=os.path.join(scisql_dir, "templates")
        configure.apply_templates(scisql_template_dir, tmp_dir)

        if configure.DEPLOY in args.step_list:

            logging.info("Deploying sciSQL")

            logging.info('Checking for mysql version')
            script=os.path.join(tmp_dir, "check_mysql_version.sh")
            utils.run_command([script])

            logging.info('Checking for mysql plugins directory')
            if not args.mysql_plugin_dir or not os.path.isdir(args.mysql_plugin_dir):
                logging.fatal('Invalid/missing MySQL plugin directory. Use --mysql-dir or --mysql-plugin-dir options.')
                sys.exit(1)

            logging.info("Deploying sciSQL shared library in {0}"
                .format(args.mysql_plugin_dir)
            )
            scisql_libname = "libscisql-{0}{1}.so".format(const.SCISQL_PREFIX, const.SCISQL_VERSION)
            # check for existing shared library file of the same version
            scisql_plugin_lib = os.path.join(
                args.mysql_plugin_dir,
                scisql_libname
            )
            if os.path.isfile(scisql_plugin_lib):
                logging.warn("Previous sciSQL library of the same version found in MySQL plugin directory ({0})"
                    .format(scisql_plugin_lib) +
                    ". sciSQL plugin (version: {0}) is certainly already deployed.".format(const.SCISQL_VERSION)
                )
                if args.force or utils.user_yes_no_query(
                    "WARNING: Do you want to replace this shared library"
                    + " by current one and continue ? Answer 'yes' only if you know what you are doing."
                ):
                        logging.warn("Forcing sciSQL deployment over an existing sciSQL plugin of the same version.")
                else:
                    logging.info("Stopping sciSQL deployment,"
                        + "please remove previous sciSQL plugin from MySQL")
                    sys.exit(1)

            scisql_lib = os.path.join(
                scisql_dir, "lib",
                scisql_libname
            )

            # Required by MariaDB: see
            # https://mariadb.com/kb/en/mariadb/create-function-udf/#upgrading-a-udf
            script = os.path.join(tmp_dir, configure.UNDEPLOY + ".sh")
            utils.run_command([script])

            shutil.copy(scisql_lib, args.mysql_plugin_dir)

            script=os.path.join(tmp_dir, configure.DEPLOY + ".sh")
            utils.run_command([script])

        if configure.TEST in args.step_list:
            script=os.path.join(tmp_dir, configure.TEST + ".sh")
            utils.run_command([script])

        if configure.UNDEPLOY in args.step_list:
            script=os.path.join(tmp_dir, configure.UNDEPLOY + ".sh")
            utils.run_command([script])

    finally:
        if logging.getLogger().getEffectiveLevel() > logging.DEBUG:
            shutil.rmtree(tmp_dir)
        else:
            logging.debug("Temporary directory {0} ".format(tmp_dir) +
                "not removed in order to enable post-mortem analysis. " +
                "Remove it manually.")

if __name__ == '__main__':
    main()

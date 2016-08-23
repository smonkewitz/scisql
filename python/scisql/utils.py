from distutils.util import strtobool
import logging
from threading import Thread
import subprocess
import sys

try:
    input = raw_input
except NameError:
    pass


def run_command(cmd_args, loglevel=logging.INFO) :
    """ Run a shell command

    Keyword arguments
    cmd_args -- a list of arguments
    logger_name -- the name of a logger, if not specified, will log to stdout

    """
    logger = logging.getLogger()

    cmd_str = ' '.join(cmd_args)
    logger.log(loglevel, "Running : {0}".format(cmd_str))

    try:

        process = subprocess.Popen(cmd_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        def logstream(stream, loggercb):
            while True:
                out = stream.readline()
                if out:
                    loggercb(out.rstrip())
                else:
                    break

        stdout_thread = Thread(
            target=logstream,
            args=(process.stdout, lambda s: logger.log(loglevel, "stdout : %s" % s))
        )

        stderr_thread = Thread(
            target=logstream,
            args=(process.stderr, lambda s: logger.log(loglevel, "stderr : %s" % s))
        )

        stdout_thread.start()
        stderr_thread.start()

        process.wait()
        stdout_thread.join()
        stderr_thread.join()

        if process.returncode != 0:
            logger.fatal("Error code returned by command : {0} ".format(cmd_str))
            sys.exit(1)

    except OSError as e:
        logger.fatal("Error : %s while running command : %s" % (e,cmd_str))
        sys.exit(1)
    except ValueError as e:
        logger.fatal("Invalid parameter : '%s' for command : %s " % (e,cmd_str))
        sys.exit(1)


def user_yes_no_query(question):
    sys.stdout.write('--\n%s [y/n]\n--\n' % question)
    while True:
        try:
            if sys.stdin.isatty():
                return strtobool(input().lower())
            else: 
                logger = logging.getLogger()
                logger.warn("Standard input isn't attached to a tty,"
                            + " using default answer (i.e. 'yes')")

                return True
        except ValueError:
            sys.stdout.write('Please respond with \'y\' or \'n\'.\n')

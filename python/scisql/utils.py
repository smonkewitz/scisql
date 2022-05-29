#! /usr/bin/env python
# encoding: utf-8
#
# Copyright (C) 2011-2022 the SciSQL authors
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

        # universal_newlines=True is to make sure that pipes are open in text mode
        # to make it return strings in Python3 using Python2-compatible API
        process = subprocess.Popen(cmd_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   universal_newlines=True)

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

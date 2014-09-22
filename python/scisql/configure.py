import ConfigParser
from distutils.util import strtobool
import logging
import os
import sys
import string

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

def getConfig():
    return config

def read_config(config_file):

    global config
    logger = logging.getLogger()
    logger.debug("Reading config file : %s" % config_file)

    if not os.path.isfile(config_file):
        logger.fatal("sciSQL build configuration file not found : %s" % config_file)
        exit(1)

    parser = ConfigParser.SafeConfigParser()
    #parser.readfp(io.BytesIO(const.DEFAULT_CONFIG))
    parser.read(config_file)

    config = dict()
    for section in parser.sections():
        config[section] = {}
        for key, val in parser.items(section):
            config[section][key] = val

    return config

def init_config(build_cfg_file, mysql_bin, mysql_user, mysql_password, mysql_socket):

    parser = read_config(build_cfg_file)
    section='mysqld'
    config[section] = dict()
    config[section]['bin'] = mysql_bin
    config[section]['user'] = mysql_user
    config[section]['pass'] = mysql_password
    config[section]['sock'] = mysql_socket

def getConfig():
    return config

def _get_template_params():
    """ Compute templates parameters from Qserv meta-configuration file
        from PATH or from environment variables for products not needed during build
    """
    logger = logging.getLogger()
    config = getConfig()

    print "XXXXXXXXXXXXXXXXX %s" % config
    print "XXXXXXXXXXXXXXx PASS : %s" % config['mysqld']['pass'] 

    params_dict = {
    'PATH': os.environ.get('PATH'),
    'MYSQL_BIN': config['mysqld']['bin'],
    'MYSQLD_SOCK': config['mysqld']['sock'],
    'MYSQLD_USER': config['mysqld']['user'],
    'MYSQLD_PASS': config['mysqld']['pass'],
    'SCISQL_PREFIX': config['scisql']['scisql_prefix'],
    'SCISQL_VERSION': config['scisql']['scisql_version'],
    'SCISQL_VSUFFIX': config['scisql']['scisql_vsuffix'],
    #'SCISQL_TMP_DIR': scisql_tmp_dir,
    }

    logger.debug("Input parameters :\n {0}".format(params_dict))

    return params_dict

def _set_perms(file):
    (path, basename) = os.path.split(file)
    script_list = [
        "xrootd.sh",
        "mysql.sh",
        "css.sh",
        "scisql.sh",
        "qserv-czar.sh"
        ]
    if (os.path.basename(path) == "bin" or
        os.path.basename(path) == "init.d" or
        basename in script_list):
        os.chmod(file, 0760)
    # all other files are configuration files
    else:
        os.chmod(file, 0660)

def apply_tpl(src_file, target_file):
    """ Creating one configuration file from one template
    """

    logger = logging.getLogger()
    logger.debug("Creating {0} from {1}".format(target_file, src_file))
    params_dict = _get_template_params()

    with open(src_file, "r") as tpl:
        t = string.Template(tpl.read())

    out_cfg = t.safe_substitute(**params_dict)
    for match in t.pattern.findall(t.template):
        name = match[1]
        if len(name) != 0 and not params_dict.has_key(name):
            logger.fatal("Template \"%s\" in file %s is not defined in configuration tool", name, src_file)
            sys.exit(1)

    dirname = os.path.dirname(target_file)
    if not os.path.exists(dirname):
        os.makedirs(os.path.dirname(target_file))
    with open(target_file, "w") as cfg:
        cfg.write(out_cfg)

def apply_templates(template_root, dest_root):

    logger = logging.getLogger()

    logger.info("Creating configuration using templates files")

    for root, dirs, files in os.walk(template_root):
        os.path.normpath(template_root)
        suffix = root[len(template_root)+1:]
        dest_dir = os.path.join(dest_root, suffix)
        for fname in files:
            src_file = os.path.join(root, fname)
            target_file = os.path.join(dest_dir, fname)

            apply_tpl(src_file, target_file)

            # applying perms
            _set_perms(target_file)

    return True

def user_yes_no_query(question):
    sys.stdout.write('\n%s [y/n]\n' % question)
    while True:
        try:
            return strtobool(raw_input().lower())
        except ValueError:
            sys.stdout.write('Please respond with \'y\' or \'n\'.\n')

def run_command(cmd_args, stdin_file=None, stdout_file=None, stderr_file=None, loglevel=logging.INFO) :
    """ Run a shell command

    Keyword arguments
    cmd_args -- a list of arguments
    logger_name -- the name of a logger, if not specified, will log to stdout

    Return a string containing stdout and stderr
    """
    logger = logging.getLogger()

    cmd_str= ' '.join(cmd_args)
    logger.log(loglevel, "Running : {0}".format(cmd_str))

    sin = None
    if stdin_file != None:
        logger.debug("stdin file : %s" % stdin_file)
        sin=open(stdin_file,"r")

    sout = None
    if stdout_file != None:
        logger.debug("stdout file : %s" % stdout_file)
        sout=open(stdout_file,"w")
    else:
        sout=subprocess.PIPE

    serr = None
    if stderr_file != None:
        logger.debug("stderr file : %s" % stderr_file)
        serr=open(stderr_file,"w")
    else:
        serr=subprocess.PIPE

# TODO : use this with python 2.7 :
#  try :
#        out = subprocess.check_output(
#                cmd_args,
#                stderr=subprocess.STDOUT
#              )
#
#    except subprocess.CalledProcessError as e:
#        logger.fatal("Error : '%s' %s  while running command : '%s'" %
#            (e,out,cmd_str)
#        )
#        sys.exit(1)

    try :
        process = subprocess.Popen(
            cmd_args, stdin=sin, stdout=sout, stderr=serr
        )

        (stdoutdata, stderrdata) = process.communicate()

        if stdoutdata != None and len(stdoutdata)>0:
            logger.info("\tstdout :\n--\n%s--" % stdoutdata)
        if stderrdata != None and len(stderrdata)>0:
            logger.info("\tstderr :\n--\n%s--" % stderrdata)

        if process.returncode!=0 :
            logger.fatal("Error code returned by command : {0} ".format(cmd_str))
            sys.exit(1)

    except OSError as e:
        logger.fatal("Error : %s while running command : %s" %
                     (e,cmd_str))
        sys.exit(1)
    except ValueError as e:
        logger.fatal("Invalid parameter : '%s' for command : %s " % (e,cmd_str))
        sys.exit(1)


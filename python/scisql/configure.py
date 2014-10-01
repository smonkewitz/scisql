from scisql import const
from distutils.util import strtobool
import logging
import os
import subprocess
import sys
import string

DEFAULT_STEP_LIST = ['deploy', 'test']
STEP_LIST = DEFAULT_STEP_LIST + ["undeploy"]
STEP_DOC = dict(
    zip(STEP_LIST,
        [
        """Deploy sciSQL plugin in a MySQL running instance""",
        """Launch tests on sciSQL plugin install """,
        """Undeploy sciSQL plugin from a MySQL running instance""",
        ]
    )
)

DEPLOY = STEP_LIST[0]
TEST = STEP_LIST[1]
UNDEPLOY = STEP_LIST[2]

def getConfig():
    return config

def init_config(scisql_base, mysql_bin, mysql_user, mysql_password, mysql_socket):

    global config
    config = dict()
    section = 'scisql'
    config[section] = dict()
    config[section]['base'] = scisql_base
    config[section]['prefix'] = const.SCISQL_PREFIX
    config[section]['version'] = const.SCISQL_VERSION
    config[section]['vsuffix'] = const.SCISQL_VSUFFIX
    section = 'mysqld'
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

    params_dict = {
    'PATH': os.environ.get('PATH'),
    'MYSQL_BIN': config['mysqld']['bin'],
    'MYSQLD_SOCK': config['mysqld']['sock'],
    'MYSQLD_USER': config['mysqld']['user'],
    'MYSQLD_PASS': config['mysqld']['pass'],
    'SCISQL_BASE': config['scisql']['base'],
    'SCISQL_PREFIX': config['scisql']['prefix'],
    'SCISQL_VERSION': config['scisql']['version'],
    'SCISQL_VSUFFIX': config['scisql']['vsuffix'],
    }

    logger.debug("Input parameters :\n {0}".format(params_dict))

    return params_dict

def _set_perms(file):
    extension = os.path.splitext(file)[1]
    if extension in [".sh"]:
        os.chmod(file, 0750)
    # all other files are configuration files
    else:
        os.chmod(file, 0640)

def apply_tpl(src_file, target_file):
    """ Creating one configuration file from one template
    """

    logger = logging.getLogger()
    logger.debug("Creating {0} from {1}".format(target_file, src_file))
    params_dict = _get_template_params()

    with open(src_file, "r") as tpl:
        t = ScisqlConfigTemplate(tpl.read())

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

class ScisqlConfigTemplate(string.Template):
    delimiter = '{{'
    pattern = r'''
    \{\{(?:
    (?P<escaped>\{\{)|
    (?P<named>[_a-z][_a-z0-9]*)\}\}|
    (?P<braced>[_a-z][_a-z0-9]*)\}\}|
    (?P<invalid>)
    )
    '''

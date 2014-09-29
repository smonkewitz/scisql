import argparse
import operator
import sys
import json

AT_LEAST_VERSION='at_least_version'
MAX_VERSION='max_version'
EXACT_VERSION='exact_version'

SCISQL_CONSTRAINT = dict()
SCISQL_CONSTRAINT[AT_LEAST_VERSION] = 5
SCISQL_CONSTRAINT[MAX_VERSION]      = 5
SCISQL_CONSTRAINT[EXACT_VERSION]    = ['5.1.65', '5.1.73']

__ver = {
    AT_LEAST_VERSION: operator.ge,
    EXACT_VERSION: operator.eq,
    MAX_VERSION: operator.le,
}

def __parse_version(version):
    return tuple(map(int, version.split('.')))

def check(version):

    ok = True
    msg = None
    try:
        mv = __parse_version(version)
    except:
        msg = 'Invalid MYSQL_SERVER_VERSION {0}'.format(version)
        ok = False

    if version in SCISQL_CONSTRAINT[EXACT_VERSION]:
        ok=True
    else:
        for constraint in [AT_LEAST_VERSION, MAX_VERSION]:
            if constraint in SCISQL_CONSTRAINT:
                try:
                        dv = __parse_version(kw[constraint])
                except:
                    msg = 'Invalid {0} value {1}'.format(constraint, kw[constraint])
                    ok = False
                if not __ver[constraint](mv, dv):
                    msg = 'MySQL server version {0} violates {1}={2}'.format(version, constraint, kw[constraint])
                    ok = False
    return (ok, msg)

def main():

    parser = argparse.ArgumentParser(
            description='''Check a given MySQL version is compatible with SciSQL''',
            formatter_class=argparse.ArgumentDefaultsHelpFormatter
            )
    parser.add_argument('-v', '--mysqlversion', help="MySQL version number")
    args = parser.parse_args()

    (ok, msg) = check(args.mysqlversion)
    if not ok:
        print("ERROR : {0}".format(msg))
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == '__main__':
    main()

from __future__ import print_function
import argparse
import operator
import sys
import re

_MIN_VERSION = 'min_version'
_MAX_VERSION = 'max_version'
_EXACT_VERSION = 'exact_version'

_MYSQL = "MySQL"
_MARIADB = "MariaDB"

# Constraints for compatible MySQL/MariaDB versions
_DB_CONSTRAINT = {
    _MYSQL: {
        _MIN_VERSION: (5, 0),
        _EXACT_VERSION: ['5.1.65', '5.1.73']
    },
    _MARIADB: {
        _MIN_VERSION: (10, 1),
        _EXACT_VERSION: ['10.1.9-MariaDB']
    },
}

_CMP = {
    _MIN_VERSION: operator.ge,
    _EXACT_VERSION: operator.eq,
    _MAX_VERSION: operator.le,
}


def _to_tuple(version):
    return tuple(int(i) for i in version.split('.'))


def _parse_version(version):
    match = re.match(r'([0-9.]+)-MariaDB', version)
    if match:
        db_name = _MARIADB
        num_version = match[1]
    else:
        db_name = _MYSQL
        num_version = version
    nums = _to_tuple(num_version)
    return db_name, nums


def check(version):
    ok = True
    msg = None
    try:
        (db_name, version_nums) = _parse_version(version)
    except ValueError:
        msg = 'Invalid MYSQL_SERVER_VERSION {0}'.format(version)
        ok = False
        return ok, msg
    db_constraints = _DB_CONSTRAINT[db_name]
    if version not in db_constraints[_EXACT_VERSION]:
        for constraint_name in [_MIN_VERSION, _MAX_VERSION]:
            if constraint_name not in db_constraints:
                continue
            constraint_nums = db_constraints[constraint_name]
            comparison_op = _CMP[constraint_name]
            if not comparison_op(version_nums, constraint_nums):
                msg = '{0} server version {1} violates {2}={3}'.format(
                    db_name, version, constraint_name, constraint_nums)
                ok = False
    return ok, msg


def main():
    parser = argparse.ArgumentParser(
        description='Check if a given MySQL/MariaDB version is compatible with SciSQL',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-v', '--mysqlversion', help="MySQL/MariaDB version number")
    args = parser.parse_args()
    (ok, msg) = check(args.mysqlversion)
    if not ok:
        print("ERROR : {0}".format(msg))
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == '__main__':
    main()

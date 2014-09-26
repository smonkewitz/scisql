import argparse
import operator
import sys
import json

__ver = {
    'atleast_version': operator.ge,
    'exact_version': operator.eq,
    'max_version': operator.le,
}

def __parse_version(version):
    return tuple(map(int, version.split('.')))

def check(version, **kw):

    ok = True
    msg = None
    try:
        mv = __parse_version(version)
    except:
        msg = 'Invalid MYSQL_SERVER_VERSION {0}'.format(version)
        ok = False
    for constraint in __ver.keys():
        if constraint in kw:
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

    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--version', help="toto")
    parser.add_argument('-c', '--constraints', nargs="+", type=str, help="titi")
    args = parser.parse_args()

    constraints=dict()
    for c in args.constraints:
        (key, value) = c.split("=")
        if key not in __ver.keys():
            print("ERROR : constraints keys must be in {0}".format( __ver.keys))
            sys.exit(1)
        else:
            constraints[key]=value
    (ok, msg) = check(args.version, **constraints)
    if not ok:
        print("ERROR : {0}".format(msg))
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == '__main__':
    main()

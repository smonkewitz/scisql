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

from future import standard_library
standard_library.install_aliases()
try:
    from ConfigParser import ConfigParser
except ImportError:
    from configparser import ConfigParser

import os
import re
import MySQLdb as sql


_udfs = ['angSep',
         's2CircleHtmRanges',
         's2CPolyHtmRanges',
         's2CPolyToBin',
         's2HtmId',
         's2HtmLevel',
         's2PtInBox',
         's2PtInCircle',
         's2PtInCPoly',
         's2PtInEllipse',
         'median',
         'percentile',
         'abMagToDn',
         'abMagToDnSigma',
         'abMagToFlux',
         'abMagToFluxSigma',
         'abMagToNanojansky',
         'abMagToNanojanskySigma',
         'dnToAbMag',
         'dnToAbMagSigma',
         'dnToFlux',
         'dnToFluxSigma',
         'fluxToAbMag',
         'fluxToAbMagSigma',
         'fluxToDn',
         'fluxToDnSigma',
         'nanojanskyToAbMag',
         'nanojanskyToAbMagSigma',
         'extractInt64',
         'raiseError',
         ]

_procs = ['s2CircleRegion',
          's2CPolyRegion',
          'grantPermissions',
          ]


def _parseMyCnf(my_cnf):
    parser = ConfigParser()
    with open(my_cnf) as conf_file:
        parser.read_file(conf_file, my_cnf)
        kw = {}
        for section in parser.sections():
            if parser.has_option(section, 'user'):
                kw['user'] = parser.get(section, 'user')
            if parser.has_option(section, 'password'):
                kw['passwd'] = parser.get(section, 'password')
            if parser.has_option(section, 'socket'):
                kw['unix_socket'] = parser.get(section, 'socket')
    return kw


def dropUdf(cursor, udf, prefix, vsuffix, libname, versioned=True):
    if versioned:
        cursor.execute('DROP FUNCTION IF EXISTS %s%s%s' % (prefix, udf, vsuffix))
    # Drop unversioned shim if it corresponds to the version of sciSQL
    # being uninstalled
    cursor.execute('''SELECT COUNT(*) FROM mysql.func
                      WHERE name = "%s%s" AND
                            dl = "%s"''' %
                   (prefix, udf, libname))
    n = cursor.fetchall()[0][0]
    if n == 1:
        cursor.execute('DROP FUNCTION IF EXISTS %s%s' % (prefix, udf))


def dropProc(cursor, proc, prefix, vsuffix):
    cursor.execute('DROP PROCEDURE IF EXISTS scisql.%s%s%s' %
                   (prefix, proc, vsuffix))
    # Drop unversioned shim if it calls the versioned procedure defined
    # by this version of sciSQL
    cursor.execute('''SELECT body_utf8 FROM mysql.proc
                      WHERE name = "%s%s" AND db = "scisql"''' % (prefix, proc))
    rows = cursor.fetchall()
    if len(rows) == 1:
        body = str(rows[0][0].decode())
        pat = r'^\s*BEGIN\s+CALL\s+%s%s%s\s*\(.*\)\s*;\s*END\s*$' % (
              prefix, proc, vsuffix)
        if re.match(pat, body):
            cursor.execute('DROP PROCEDURE IF EXISTS scisql.%s%s' %
                           (prefix, proc))


def uninstall():
    prefix = os.environ['SCISQL_PREFIX']
    vsuffix = os.environ['SCISQL_VSUFFIX']
    libname = os.environ['SCISQL_LIBNAME']
    conn = sql.connect(**_parseMyCnf(os.environ['MYSQL_CNF']))
    try:
        cursor = conn.cursor()
        try:
            cursor.execute('USE mysql')
            for udf in _udfs:
                dropUdf(cursor, udf, prefix, vsuffix, libname)
            dropUdf(cursor, 'getVersion', prefix, vsuffix, libname, False)
            for proc in _procs:
                dropProc(cursor, proc, prefix, vsuffix)
            cursor.execute('DROP DATABASE IF EXISTS scisql_demo')
        except:
            cursor.close()
            raise
    except:
        conn.close()
        raise


if __name__ == '__main__':
    uninstall()

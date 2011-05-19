#! /usr/bin/env python

#
# Copyright (C) 2011 Serge Monkewitz
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License v3 as published
# by the Free Software Foundation, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
# A copy of the LGPLv3 is available at <http://www.gnu.org/licenses/>.
#
# Authors:
#     - Serge Monkewitz, IPAC/Caltech
#
# Work on this project has been sponsored by LSST and SLAC/DOE.
#
from __future__ import with_statement

import getpass
import math
import os
import unittest

import MySQLdb as sql


def flatten(l, ltypes=(list, tuple)):
    ltype = type(l)
    l = list(l)
    i = 0
    while i < len(l):
        while isinstance(l[i], ltypes):
            if not l[i]:
                l.pop(i)
                i -= 1
                break
            else:
                l[i:i + 1] = l[i]
        i += 1
    return ltype(l)

class ColumnName(str):
    pass

def dbparam(x):
    if x is None:
        return 'NULL'
    elif isinstance(x, ColumnName):
        return "`" + x + "`"
    elif isinstance(x, basestring):
        return "'" + x + "'"
    else:
        return repr(x)

def _parseMyCnf(my_cnf):
    if not isinstance(my_cnf, basestring):
        raise RuntimeError('invalid MySQL options file path')
    kw = {}
    with open(my_cnf, 'rb') as f:
        for line in f:
           kv = [s.strip() for s in line.split('=')]
           if len(kv) == 2:
               if kv[0] == 'user':
                   kw['user'] = kv[1]
               elif kv[0] == 'password':
                   kw['passwd'] = kv[1]
               elif kv[0] == 'socket':
                   kw['unix_socket'] = kv[1]
    return kw


class MySqlUdfTestCase(unittest.TestCase):
    """Base class for MySQL UDF test-cases.
    """
    @classmethod
    def setUpClass(self):
        connkw = _parseMyCnf(os.environ['MYSQL_CNF'])
        self._conn = sql.connect(**connkw)
        try:
            self._cursor = self._conn.cursor()
            try:
                self._cursor.execute("CREATE DATABASE IF NOT EXISTS test_scisql")
                self._cursor.execute("USE test_scisql")
            except:
                self._cursor.close()
                raise
        except:
            self._conn.close()
            raise

    @classmethod
    def tearDownClass(self):
        self._cursor.close()
        self._conn.close()

    def query(self, stmt):
        self._cursor.execute(stmt)
        return self._cursor.fetchall()

    def tempTable(self, name, cols):
        return TempTable(self._cursor, name, cols)


class TempTable(object):
    """A temporary MySQL table.
    """
    def __init__(self, cursor, name, cols):
        self._cursor = cursor
        self._name = name
        self._cols = list(cols)
        self._cursor.execute("CREATE TEMPORARY TABLE %s (%s)" % (name, ','.join(self._cols)))

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.drop()
        return False

    def drop(self):
        self._cursor.execute("DROP TABLE IF EXISTS " +  self._name)

    def insert(self, row):
        self._cursor.execute("INSERT INTO %s VALUES (%s)" %
            (self._name, ",".join(["%s"] * len(self._cols))), row)

    def insertMany(self, rows):
        self._cursor.executemany("INSERT INTO %s VALUES (%s)" %
            (self._name, ",".join(["%s"] * len(self._cols))), rows)


def angSep(ra1, dec1, ra2, dec2):
    sdt = math.sin(math.radians(ra1 - ra2) * 0.5)
    sdp = math.sin(math.radians(dec1 - dec2) * 0.5)
    cc = math.cos(math.radians(dec1)) * math.cos(math.radians(dec2))
    s = math.sqrt(sdp * sdp + cc * sdt * sdt)
    if s > 1.0:
        return 180.0
    else:
        return 2.0 * math.degrees(math.asin(s))


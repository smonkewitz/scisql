#! /usr/bin/env python
# encoding: utf-8
#
# Copyright (C) 2011 Serge Monkewitz
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

import math
import os
import unittest

try:
    from ConfigParser import ConfigParser
except ImportError:
    from configparser import ConfigParser

try:
   stringTypes = (str, unicode)
except TypeError:
   stringTypes = (str,)

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
    elif isinstance(x, bytes):
        return "'" + str(x.decode()) + "'"
    elif isinstance(x, stringTypes):
        return "'" + x + "'"
    else:
        return repr(x)


def _parseMyCnf(my_cnf):
    parser = ConfigParser()
    with open(my_cnf) as conf_file:
        parser.readfp(conf_file, my_cnf)
        kw = {}
        for section in parser.sections():
            for key, val in parser[section].items():
                if key == 'user':
                    kw['user'] = val
                elif key == 'password':
                    kw['passwd'] = val
                elif key == 'socket':
                    kw['unix_socket'] = val
    return kw


class MySqlUdfTestCase(unittest.TestCase):
    """Base class for MySQL UDF test-cases.
    """
    def setUp(self):
        self._prefix = os.environ['SCISQL_PREFIX']
        connkw = _parseMyCnf(os.environ['MYSQL_CNF'])
        self._conn = sql.connect(**connkw)
        try:
            self._cursor = self._conn.cursor()
            try:
                self._cursor.execute("CREATE DATABASE IF NOT EXISTS scisql_test")
                self._cursor.execute("USE scisql_test")
            except:
                self._cursor.close()
                raise
        except:
            self._conn.close()
            raise

    def tearDown(self):
        self._cursor.close()
        self._conn.close()
        del self._cursor
        del self._conn

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
        self._cursor.execute("DROP TABLE IF EXISTS " + self._name)

    def insert(self, row):
        self._cursor.execute("INSERT INTO %s VALUES (%s)" %
                             (self._name, ",".join(["%s"] * len(self._cols))),
                             row)

    def insertMany(self, rows):
        self._cursor.executemany("INSERT INTO %s VALUES (%s)" %
                                 (self._name, ",".join(["%s"] * len(self._cols))),
                                 rows)


def angSep(ra1, dec1, ra2, dec2):
    sdt = math.sin(math.radians(ra1 - ra2) * 0.5)
    sdp = math.sin(math.radians(dec1 - dec2) * 0.5)
    cc = math.cos(math.radians(dec1)) * math.cos(math.radians(dec2))
    s = math.sqrt(sdp * sdp + cc * sdt * sdt)
    if s > 1.0:
        return 180.0
    else:
        return 2.0 * math.degrees(math.asin(s))

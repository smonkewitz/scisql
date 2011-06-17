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

from __future__ import with_statement

import random
import sys
import unittest

from base import *


class S2CPolyTestCase(MySqlUdfTestCase):
    """s2PtInCPoly() and s2CPolyToBin() UDF test-case.
    """
    def setUp(self):
        random.seed(123456789)
        x = (0, 0);  nx = (180, 0)
        y = (90, 0); ny = (270, 0)
        z = (0, 90); nz = (0, -90)
        self._tris = [(x, y, z),
                      (y, nx, z),
                      (nx, ny, z),
                      (ny, (360, 0), z),
                      ((360, 0), ny, nz),
                      (ny, nx, nz),
                      (nx, y, nz),
                      (y, x, nz)]
        super(S2CPolyTestCase, self).setUp()

    def _s2PtInCPoly(self, result, *args):
        stmt = "SELECT %ss2PtInCPoly(%s)" % (self._prefix, ",".join(map(dbparam, args)))
        rows = self.query(stmt)
        self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
        self.assertEqual(rows[0][0], result, stmt + " did not return " + repr(result))

    def _s2PtInCPolyBin(self, result, ra, dec, *args):
        stmt = "SELECT %ss2PtInCPoly(%s, %s, %ss2CPolyToBin(%s))" % (
            self._prefix, dbparam(ra), dbparam(dec),
            self._prefix, ",".join(map(dbparam, args)))
        rows = self.query(stmt)
        self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
        self.assertEqual(rows[0][0], result, stmt + " did not return " + repr(result))

    def testConstArgs(self):
        """Test with constant arguments.
        """
        for i in xrange(8):
            a = [0, 0, 0, 0, 90, 0, 0, 90]; a[i] = None
            self._s2PtInCPoly(0, *a)
            self._s2PtInCPolyBin(0, *a)
        for d in (-91, 91):
            a = [0, d, 0, 0, 90, 0, 0, 90]
            self._s2PtInCPoly(None, *a)
            self._s2PtInCPolyBin(None, *a)
            a = [0, 0, 0, 0, 90, 0, 0, d]
            self.assertRaises(Exception, self._s2PtInCPoly, [self, None] + a)
            self.assertRaises(Exception, self._s2PtInCPolyBin, [self, None] + a)

        self.assertRaises(Exception, self._s2PtInCPoly,
                          (self, None, 0, 0, 0, 0, 90, 0, 60, 45, 30))
        self.assertRaises(Exception, self._s2PtInCPoly,
                          (self, None, 0.0, 0.0, 0, 0, 90, 0, 60))
        self.assertRaises(Exception, self._s2PtInCPoly,
                          (self, None, 0, 0, 0, 0, 90, 0))
        self.assertRaises(Exception, self._s2PtInCPolyBin,
                          (self, None, 0, 0, 0, 0, 90, 0, 60, 45, 30))
        self.assertRaises(Exception, self._s2PtInCPolyBin,
                          (self, None, 0.0, 0.0, 0, 0, 90, 0, 60))
        self.assertRaises(Exception, self._s2PtInCPolyBin,
                          (self, None, 0, 0, 0, 0, 90, 0))
        for t in self._tris:
            for i in xrange(100):
                ra = random.uniform(0.0, 360.0)
                dec = random.uniform(-90.0, 90.0)
                if ((t[2][1] > 0 and (dec < 0.0 or ra < t[0][0] or ra > t[1][0])) or
                    (t[2][1] < 0 and (dec > 0.0 or ra < t[1][0] or ra > t[0][0]))):
                    self._s2PtInCPoly(0, ra, dec, *flatten(t))
                    self._s2PtInCPolyBin(0, ra, dec, *flatten(t))
                else:
                    self._s2PtInCPoly(1, ra, dec, *flatten(t))
                    self._s2PtInCPolyBin(1, ra, dec, *flatten(t))
        # Test with vertices specified in clockwise order
        for t in self._tris:
            rt = tuple(reversed(t))
            for i in xrange(100):
                ra = random.uniform(0.0, 360.0)
                dec = random.uniform(-90.0, 90.0)
                if ((t[2][1] > 0 and (dec < 0.0 or ra < t[0][0] or ra > t[1][0])) or
                    (t[2][1] < 0 and (dec > 0.0 or ra < t[1][0] or ra > t[0][0]))):
                    self._s2PtInCPoly(0, ra, dec, *flatten(rt))
                    self._s2PtInCPolyBin(0, ra, dec, *flatten(rt))
                else:
                    self._s2PtInCPoly(1, ra, dec, *flatten(rt))
                    self._s2PtInCPolyBin(1, ra, dec, *flatten(rt))

    def testColumnArgs(self):
        """Test with arguments taken from a table.
        """
        with self.tempTable("S2CPoly", ("inside INTEGER",
                                        "ra DOUBLE PRECISION",
                                        "decl DOUBLE PRECISION",
                                        "ra1 DOUBLE PRECISION",
                                        "decl1 DOUBLE PRECISION",
                                        "ra2 DOUBLE PRECISION",
                                        "decl2 DOUBLE PRECISION",
                                        "ra3 DOUBLE PRECISION",
                                        "decl3 DOUBLE PRECISION",
                                        "poly VARBINARY(255)")) as t:
            inFirst = 0
            for t in self._tris:
                for i in xrange(100):
                    ra = random.uniform(0.0, 360.0)
                    dec = random.uniform(-90.0, 90.0)
                    if ((t[2][1] > 0 and (dec < 0.0 or ra < t[0][0] or ra > t[1][0])) or
                        (t[2][1] < 0 and (dec > 0.0 or ra < t[1][0] or ra > t[0][0]))):
                        inside = 0
                    else:
                        inside = 1
                    tf = self._tris[0]
                    if not ((tf[2][1] > 0 and (dec < 0.0 or ra < tf[0][0] or ra > tf[1][0])) or
                            (tf[2][1] < 0 and (dec > 0.0 or ra < tf[1][0] or ra > tf[0][0]))):
                        inFirst += 1
                    stmt = """INSERT INTO S2CPoly
                              VALUES(%s, %s, %s, %s, %s, %s, %s, %s, %s, NULL)""" % (
                               (inside, ra, dec) + flatten(t))
                    self._cursor.execute(stmt)
            stmt = """UPDATE S2CPoly SET poly = %ss2CPolyToBin(
                          ra1, decl1, ra2, decl2, ra3, decl3)""" % self._prefix
            self._cursor.execute(stmt)
            # Test with constant position
            stmt = """SELECT COUNT(*) FROM S2CPoly
                      WHERE %ss2PtInCPoly(45, 45, poly) = 1""" % self._prefix
            rows = self.query(stmt)
            self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
            self.assertEqual(rows[0][0], 100, stmt + " did not return 100")
            stmt = """SELECT COUNT(*) FROM S2CPoly
                      WHERE %ss2PtInCPoly(45, 45,
                          ra1, decl1, ra2, decl2, ra3, decl3) = 1""" % self._prefix
            rows = self.query(stmt)
            self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
            self.assertEqual(rows[0][0], 100, stmt + " did not return 100")
            # Test with constant polygon
            stmt = """SELECT COUNT(*) FROM S2CPoly
                      WHERE %ss2PtInCPoly(ra, decl, %s) = 1""" % (
                   self._prefix, ",".join(map(dbparam, flatten(self._tris[0]))))
            rows = self.query(stmt)
            self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
            self.assertEqual(rows[0][0], inFirst, "%s did not return %d" % (stmt, inFirst))
            stmt = """SELECT COUNT(*) FROM S2CPoly
                      WHERE %ss2PtInCPoly(ra, decl, %ss2CPolyToBin(%s)) = 1""" % (
                   self._prefix, self._prefix, ",".join(map(dbparam, flatten(self._tris[0]))))
            rows = self.query(stmt)
            self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
            self.assertEqual(rows[0][0], inFirst, "%s did not return %d" % (stmt, inFirst))
            # Test with varying position and polygon
            stmt = """SELECT COUNT(*) FROM S2CPoly
                      WHERE %ss2PtInCPoly(ra, decl,
                          ra1, decl1, ra2, decl2, ra3, decl3) != inside""" % self._prefix
            rows = self.query(stmt)
            self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
            self.assertEqual(rows[0][0], 0, stmt + " did not return 0")
            stmt = """SELECT COUNT(*) FROM S2CPoly
                      WHERE %ss2PtInCPoly(ra, decl, poly) != inside""" % self._prefix
            self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
            self.assertEqual(rows[0][0], 0, stmt + " did not return 0")


if __name__ == "__main__":
    suite = unittest.makeSuite(S2CPolyTestCase)
    runner = unittest.TextTestRunner()
    if not runner.run(suite).wasSuccessful():
        sys.exit(1)


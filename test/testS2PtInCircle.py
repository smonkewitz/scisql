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

import math
import random
import sys
import unittest

from base import *


class S2PtInCircleTestCase(MySqlUdfTestCase):
    """s2PtInCircle() UDF test-case.
    """
    def setUp(self):
        random.seed(123456789)
        super(S2PtInCircleTestCase, self).setUp()

    def _s2PtInCircle(self, result, *args):
        stmt = "SELECT %ss2PtInCircle(%s)" % (self._prefix, ",".join(map(dbparam, args)))
        rows = self.query(stmt)
        self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
        self.assertEqual(rows[0][0], result, stmt + " did not return " + repr(result))

    def testConstArgs(self):
        """Test with constant arguments.
        """
        for i in range(5):
            a = [0.0]*5
            a[i] = None
            self._s2PtInCircle(0, *a)
        for d in (-91.0, 91.0):
            self._s2PtInCircle(None, 0.0, d, 0.0, 0.0, 0.0)
            self._s2PtInCircle(None, 0.0, 0.0, 0.0, d, 0.0)
        for r in (-1.0, 181.0):
            self._s2PtInCircle(None, 0.0, 0.0, 0.0, 0.0, r)
        for i in range(10):
            ra_cen = random.uniform(0.0, 360.0)
            dec_cen = random.uniform(-90.0, 90.0)
            radius = random.uniform(0.0001, 10.0)
            for j in range(100):
                delta = radius / math.cos(math.radians(dec_cen))
                ra = random.uniform(ra_cen - delta, ra_cen + delta)
                dec = random.uniform(max(dec_cen - radius, -90.0),
                                     min(dec_cen + radius, 90.0))
                r = angSep(ra_cen, dec_cen, ra, dec)
                if r < radius - 1e-9:
                    self._s2PtInCircle(1, ra, dec, ra_cen, dec_cen, radius)
                elif r > radius + 1e-9:
                    self._s2PtInCircle(0, ra, dec, ra_cen, dec_cen, radius)

    def testColumnArgs(self):
        """Test with argument taken from a table.
        """
        with self.tempTable("S2PtInCircle", ("inside INTEGER",
                                             "ra DOUBLE PRECISION",
                                             "decl DOUBLE PRECISION",
                                             "cenRa DOUBLE PRECISION",
                                             "cenDecl DOUBLE PRECISION",
                                             "radius DOUBLE PRECISION")) as t:
            # Test with constant radius
            t.insert((1, 0.0, 0.0, 0.0, 0.0, 1.0))
            t.insert((0, 1.0, 1.0, 0.0, 0.0, 1.0))
            stmt = """SELECT COUNT(*) FROM S2PtInCircle
                      WHERE inside != %ss2PtInCircle(
                          ra, decl, cenRa, cenDecl, 1.0)""" % self._prefix
            rows = self.query(stmt)
            self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
            self.assertEqual(rows[0][0], 0, "%s detected %d disagreements" % (stmt, rows[0][0]))
            # Add many more rows
            for i in range(1000):
                ra_cen = random.uniform(0.0, 360.0)
                dec_cen = random.uniform(-90.0, 90.0)
                radius = random.uniform(0.0001, 10.0)
                delta = radius / math.cos(math.radians(dec_cen))
                ra = random.uniform(ra_cen - delta, ra_cen + delta)
                dec = random.uniform(max(dec_cen - radius, -90.0),
                                     min(dec_cen + radius, 90.0))
                r = angSep(ra_cen, dec_cen, ra, dec)
                if r < radius - 1e-9:
                    t.insert((1, ra, dec, ra_cen, dec_cen, radius))
                elif r > radius + 1e-9:
                    t.insert((0, ra, dec, ra_cen, dec_cen, radius))
            # Test without any constant arguments
            stmt = """SELECT COUNT(*) FROM S2PtInCircle
                      WHERE inside != %ss2PtInCircle(
                          ra, decl, cenRa, cenDecl, radius)""" % self._prefix
            rows = self.query(stmt)
            self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
            self.assertEqual(rows[0][0], 0, "%s detected %d disagreements" % (stmt, rows[0][0]))


if __name__ == "__main__":
    suite = unittest.makeSuite(S2PtInCircleTestCase)
    runner = unittest.TextTestRunner()
    if not runner.run(suite).wasSuccessful():
        sys.exit(1)

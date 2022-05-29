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

import random
import sys
import unittest

from base import *


class AngSepTestCase(MySqlUdfTestCase):
    """angSep() UDF test-case.
    """
    def setUp(self):
        random.seed(123456789)
        super(AngSepTestCase, self).setUp()

    def _angSep(self, result, *args):
        stmt = "SELECT %sangSep(%s)" % (self._prefix, ",".join(map(dbparam, args)))
        rows = self.query(stmt)
        self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
        if result is None:
            self.assertEqual(rows[0][0], None, stmt + " did not return NULL.")
        else:
            self.assertAlmostEqual(rows[0][0], result, 11,
                                   stmt + " not close enough to %s" % dbparam(result))

    def testConstArgs(self):
        """Test with constant arguments.
        """
        for i in range(4):
            a = [0.0]*4
            a[i] = None
            self._angSep(None, *a)
        for d in (-91.0, 91.0):
            self._angSep(None, 0.0, d, 0.0, 0.0)
            self._angSep(None, 0.0, 0.0, 0.0, d)
        for d in (0.0, 90.0, -90.0):
            self._angSep(0.0, 0.0, d, 0.0, d)
        for i in range(100):
            args = [random.uniform(0.0, 360.0),
                    random.uniform(-90.0, 90.0),
                    random.uniform(0.0, 360.0),
                    random.uniform(-90.0, 90.0)]
            self._angSep(angSep(*args), *args)

    def testColumnArgs(self):
        """Test with arguments taken from a table.
        """
        with self.tempTable("AngSep", ("i INTEGER",
                                       "ra1 DOUBLE PRECISION",
                                       "decl1 DOUBLE PRECISION",
                                       "ra2 DOUBLE PRECISION",
                                       "decl2 DOUBLE PRECISION")) as t:
            rows = [(i,
                     random.uniform(0.0, 360.0),
                     random.uniform(-90.0, 90.0),
                     random.uniform(0.0, 360.0),
                     random.uniform(-90.0, 90.0)) for i in range(100)]
            t.insertMany(rows)
            stmt = """SELECT i, %sangSep(ra1, decl1, ra2, decl2) FROM AngSep
                      ORDER BY i""" % self._prefix
            for res in self.query(stmt):
                self.assertAlmostEqual(res[1], angSep(*rows[res[0]][1:]), 11,
                                       "angSep(" + ",".join(map(repr, rows[res[0]][1:])) +
                                       "): Python and MySQL UDF don't agree to 11 decimal places")


if __name__ == "__main__":
    suite = unittest.makeSuite(AngSepTestCase)
    runner = unittest.TextTestRunner()
    if not runner.run(suite).wasSuccessful():
        sys.exit(1)

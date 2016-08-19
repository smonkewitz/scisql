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

import random
import sys
import unittest

from base import *


class MedianTestCase(MySqlUdfTestCase):
    """median() UDF test-case.
    """
    def setUp(self):
        random.seed(123456789)
        super(MedianTestCase, self).setUp()

    def testDistinct(self):
        """Test small and large sequences of distinct values.
        """
        for n in (100, 10000):
            with self.tempTable("Median", ("x DOUBLE PRECISION",)) as t:
                values = [(v,) for v in range(n)]
                random.shuffle(values)
                t.insertMany(values)
                stmt = "SELECT %smedian(x) FROM Median" % self._prefix
                rows = self.query(stmt)
                median = 0.5 * (n - 1)
                self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
                self.assertAlmostEqual(
                    rows[0][0], median, 15,
                    "median() of integers [0 .. %d) not close enough to %f" % (n, median))
                t.insert(n)
                median = 0.5 * n
                rows = self.query(stmt)
                self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
                self.assertAlmostEqual(
                    rows[0][0], median, 15,
                    "median() of integers from [0 .. %d] not close enough to %f" % (n, median))

    def testIdentical(self):
        for n in (100, 10000):
            with self.tempTable("Median", ("x DOUBLE PRECISION",)) as t:
                t.insertMany([(1,)]*n)
                stmt = "SELECT %smedian(x) FROM Median" % self._prefix
                rows = self.query(stmt)
                self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
                self.assertAlmostEqual(
                    rows[0][0], 1.0, 15,
                    "median() of %d ones not close enough to one" % n)


if __name__ == "__main__":
    suite = unittest.makeSuite(MedianTestCase)
    runner = unittest.TextTestRunner()
    if not runner.run(suite).wasSuccessful():
        sys.exit(1)

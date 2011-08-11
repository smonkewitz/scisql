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

import math
import random
import sys
import unittest

from base import *


class PercentileTestCase(MySqlUdfTestCase):
    """percentile() UDF test-case.
    """
    def setUp(self):
        random.seed(123456789)
        super(PercentileTestCase, self).setUp()

    def testDistinct(self):
        """Test small and large sequences of distinct values.
        """
        for n in (101, 10001):
            with self.tempTable("Percentile", ("x DOUBLE PRECISION",)) as t:
                values = [(v,) for v in xrange(n)]
                random.shuffle(values)
                t.insertMany(values)
                stmt = "SELECT %spercentile(x, 25) FROM Percentile" % self._prefix
                rows = self.query(stmt)
                quartile = math.floor(0.25 * (n - 1))
                self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
                self.assertAlmostEqual(rows[0][0], quartile, 15,
                    "quartile of integers [0 .. %d] not close enough to %f" % (n - 1, quartile))
                t.insert((n))
                quartile += 0.25
                rows = self.query(stmt)
                self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
                self.assertAlmostEqual(rows[0][0], quartile, 15,
                    "quartile of integers [0 .. %d] not close enough to %f" % (n, quartile))

    def testGroups(self):
        with self.tempTable("Percentile", ("grp INTEGER",
                                           "percent TINYINT",
                                           "x DOUBLE PRECISION")) as t:
            for grp in xrange(3):
                values = [(grp, grp*25, v) for v in xrange(101)]
                random.shuffle(values)
                t.insertMany(values)
            stmt = "SELECT %spercentile(x, percent) FROM Percentile GROUP BY grp" % self._prefix
            rows = self.query(stmt)
            self.assertEqual(len(rows), 3, stmt + " did not return 3 rows")
            self.assertAlmostEqual(rows[0][0], 0.0, 15)
            self.assertAlmostEqual(rows[1][0], 25.0, 15)
            self.assertAlmostEqual(rows[2][0], 50.0, 15)


if __name__ == "__main__":
    suite = unittest.makeSuite(PercentileTestCase)
    runner = unittest.TextTestRunner()
    if not runner.run(suite).wasSuccessful():
        sys.exit(1)


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
                stmt = "SELECT percentile(x, 25) FROM Percentile"
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
            stmt = "SELECT percentile(x, percent) FROM Percentile GROUP BY grp"
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


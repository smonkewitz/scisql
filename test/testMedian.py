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
# ----------------------------------------------------------------
#
# Tests for the median() UDF.
#

from __future__ import with_statement
import random

from base import *


class MedianTestCase(MySqlUdfTestCase):
    """median() UDF test-case.
    """
    def setUp(self):
        random.seed(123456789)

    def testDistinct(self):
        """Test small and large sequences of distinct values.
        """
        for n in (100, 10000):
            with self.tempTable("Median", ("x DOUBLE PRECISION",)) as t:
                values = [(v,) for v in xrange(n)]
                random.shuffle(values)
                t.insertMany(values)
                stmt = "SELECT median(x) FROM Median"
                rows = self.query(stmt)
                median = 0.5 * (n - 1)
                self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
                self.assertAlmostEqual(rows[0][0], median, 15,
                    "median() of integers [0 .. %d) not close enough to %f" % (n, median))
                t.insert((n))
                median = 0.5 * n
                rows = self.query(stmt)
                self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
                self.assertAlmostEqual(rows[0][0], median, 15,
                    "median() of integers from [0 .. %d] not close enough to %f" % (n, median))

    def testIdentical(self):
        for n in (100, 10000):
            with self.tempTable("Median", ("x DOUBLE PRECISION",)) as t:
                t.insertMany([(1,)]*n)
                stmt = "SELECT median(x) FROM Median"
                rows = self.query(stmt)
                self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
                self.assertAlmostEqual(rows[0][0], 1.0, 15,
                    "median() of %d ones not close enough to one" % n)


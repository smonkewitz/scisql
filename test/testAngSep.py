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
# Tests for the angSep() UDF.
#

from __future__ import with_statement
import random
import sys
import unittest

from base import *


class AngSepTestCase(MySqlUdfTestCase):
    """angSep() UDF test-case.
    """
    def setUp(self):
        random.seed(123456789)

    def _angSep(self, result, *args):
        stmt = "SELECT angSep(%s, %s, %s, %s)" % tuple(map(dbparam, args))
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
        for i in xrange(4):
            a = [0.0]*4; a[i] = None
            self._angSep(None, *a)
        for d in (-91.0, 91.0):
            self._angSep(None, 0.0, d, 0.0, 0.0)
            self._angSep(None, 0.0, 0.0, 0.0, d)
        for d in (0.0, 90.0, -90.0):
            self._angSep(0.0, 0.0, d, 0.0, d)
        for i in xrange(100):
            args = [ random.uniform(0.0, 360.0),
                     random.uniform(-90.0, 90.0),
                     random.uniform(0.0, 360.0),
                     random.uniform(-90.0, 90.0) ]
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
                     random.uniform(-90.0, 90.0)) for i in xrange(100)]
            t.insertMany(rows)
            stmt = "SELECT i, angSep(ra1, decl1, ra2, decl2) FROM AngSep ORDER BY i"
            for res in self.query(stmt):
                self.assertAlmostEqual(res[1], angSep(*rows[res[0]][1:]), 11,
                    "angSep(" + ",".join(map(repr, rows[res[0]][1:])) +
                    "): Python and MySQL UDF do not agree to 11 decimal places")


if __name__ == "__main__":
    suite = unittest.makeSuite(AngSepTestCase)
    runner = unittest.TextTestRunner()
    if not runner.run(suite).wasSuccessful():
        sys.exit(1)


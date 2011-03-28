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

import random

from base import *


class AngSepTestCase(MySqlUdfTestCase):
    """angSep() UDF test-case.
    """
    def setUp(self):
        random.seed(123456789)

    def _angSep(self, result, *args):
        query = "SELECT angSep(%s, %s, %s, %s)" % tuple(map(dbparam, args))
        self._cursor.execute(query)
        rows = self._cursor.fetchall()
        if result is None:
            self.assertEqual(rows[0][0], result, query + " did not return NULL.")
        else:
            msg = query + " not close enough to %s" % dbparam(result)
            self.assertAlmostEqual(rows[0][0], result, 11, msg)

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
        pass


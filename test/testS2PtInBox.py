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
# Tests for the s2PtInBox() UDF.
#

from base import *



class S2PtInBoxTestCase(MySqlUdfTestCase):
    """s2PtInBox() UDF test-case. Test with constant arguments only,
    since the implementation does not perform caching of any sort.
    """
    def _s2PtInBox(self, result, *args):
        stmt = "SELECT s2PtInBox(%s, %s, %s, %s, %s, %s)" % tuple(map(dbparam, args))
        rows = self.query(stmt)
        self.assertEquals(len(rows), 1, stmt + " returned multiple rows")
        self.assertEquals(rows[0][0], result, stmt + " did not return " + repr(result))

    def testConstArgs(self):
        """Test UDF with constant arguments.
        """
        for i in xrange(6):
            a = [0.0]*6; a[i] = None
            self._s2PtInBox(0, *a)
        for d in (-91.0, 91.0):
            for i in (1, 3, 5):
                a = [0.0]*6; a[i] = d
                self._s2PtInBox(None, *a)
        for ra_min, ra_max in ((370.0, 10.0), (50.0, -90.0), (400.0, -400.0)):
            self._s2PtInBox(None, 0.0, 0.0, ra_min, 0.0, ra_max, 0.0)
        for ra, dec in ((360.0, 0.5), (720.0, 0.5), (5.0, 0.5), (355.0, 0.5)):
            self._s2PtInBox(1, ra, dec, 350.0, 0.0, 370.0, 1.0)
        for ra, dec in ((0.0, 1.1), (0.0, -0.1), (10.1, 0.5), (349.9, 0.5)):
            self._s2PtInBox(0, ra, dec, 350.0, 0.0, 370.0, 1.0)


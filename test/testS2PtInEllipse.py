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
# Tests for the s2PtInEllipse() UDF.
#

import math
import random

from base import *


def angSep(ra1, dec1, ra2, dec2):
    sdt = math.sin(math.radians(ra1 - ra2) * 0.5)
    sdp = math.sin(math.radians(dec1 - dec2) * 0.5)
    cc = math.cos(math.radians(dec1)) * math.cos(math.radians(dec2))
    s = math.sqrt(sdp * sdp + cc * sdt * sdt)
    if s > 1.0:
        return 180.0
    else:
        return 2.0 * math.degrees(math.asin(s))

def s2PtInEllipse(ra, dec, ra_cen, dec_cen, smaa, smia, ang):
    ra = math.radians(ra)
    dec = math.radians(dec)
    v = (math.cos(ra) * math.cos(dec),
         math.sin(ra) * math.cos(dec),
         math.sin(dec))
    theta = math.radians(ra_cen)
    phi = math.radians(dec_cen)
    ang = math.radians(ang)
    sinTheta = math.sin(theta)
    cosTheta = math.cos(theta)
    sinPhi = math.sin(phi)
    cosPhi = math.cos(phi)
    sinAng = math.sin(ang)
    cosAng = math.cos(ang)
    # get coords of input point in (N,E) basis
    n = cosPhi * v[2] - sinPhi * (sinTheta * v[1] + cosTheta * v[0])
    e = cosTheta * v[1] - sinTheta * v[0]
    # rotate by negated major axis angle
    x = sinAng * e + cosAng * n
    y = cosAng * e - sinAng * n
    # scale by inverse of semi-axis-lengths
    x /= math.radians(smaa / 3600.0)
    y /= math.radians(smia / 3600.0)
    # Apply point in circle test for the unit circle centered at the origin
    r = x * x + y * y
    if r < 1.0 - 1e-11:
        return True
    elif r > 1.0 + 1e-11:
        return False
    return None


class S2PtInEllipseTestCase(MySqlUdfTestCase):
    """s2PtInEllipse() UDF test-case.
    """
    def setUp(self):
        random.seed(123456789)

    def _s2PtInEllipse(self, result, *args):
        stmt = "SELECT s2PtInEllipse(%s, %s, %s, %s, %s, %s, %s)" % tuple(map(dbparam, args))
        self.query(stmt, result)

    def testConstArgs(self):
        """Test with constant arguments.
        """
        for i in xrange(7):
            a = [0.0]*7; a[i] = None
            self._s2PtInEllipse(0, *a)
        for d in (-91.0, 91.0):
            self._s2PtInEllipse(None, 0.0, d, 0.0, 0.0, 0.0, 0.0, 0.0)
            self._s2PtInEllipse(None, 0.0, 0.0, 0.0, d, 0.0, 0.0, 0.0)
        self._s2PtInEllipse(None, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0)
        self._s2PtInEllipse(None, 0.0, 0.0, 0.0, 0.0, 2.0, -1.0, 0.0)
        self._s2PtInEllipse(None, 0.0, 0.0, 0.0, 0.0, 36001.0, 1.0, 0.0)
        for i in xrange(10):
            ra_cen = random.uniform(0.0, 360.0)
            dec_cen = random.uniform(-90.0, 90.0)
            smaa = random.uniform(0.0001, 36000.0)
            smia = random.uniform(0.00001, smaa)
            ang = random.uniform(-180.0, 180.0)
            for j in xrange(100):
                smaaDeg = smaa / 3600.0
                delta = smaaDeg / math.cos(math.radians(dec_cen))
                ra = random.uniform(ra_cen - delta, ra_cen + delta)
                dec = random.uniform(max(dec_cen - smaaDeg, -90.0),
                                     min(dec_cen + smaaDeg, 90.0))
                r = s2PtInEllipse(ra, dec, ra_cen, dec_cen, smaa, smia, ang)
                if r is True:
                    self._s2PtInEllipse(1, ra, dec, ra_cen, dec_cen, smaa, smia, ang)
                elif r is False:
                    self._s2PtInEllipse(0, ra, dec, ra_cen, dec_cen, smaa, smia, ang)

    def testColumnArgs(self):
        """Test with arguments taken from a table.
        """
        pass


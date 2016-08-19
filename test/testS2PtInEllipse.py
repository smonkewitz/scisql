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

import math
import random
import sys
import unittest

from base import *


def s2PtInEllipse(ra, dec, ra_cen, dec_cen, smaa, smia, ang):
    ra = math.radians(ra)
    dec = math.radians(dec)
    v = (math.cos(ra) * math.cos(dec),
         math.sin(ra) * math.cos(dec),
         math.sin(dec))
    theta = math.radians(ra_cen)
    phi = math.radians(dec_cen)
    ang = math.radians(ang)
    sin_theta = math.sin(theta)
    cos_theta = math.cos(theta)
    sin_phi = math.sin(phi)
    cos_phi = math.cos(phi)
    sin_ang = math.sin(ang)
    cos_ang = math.cos(ang)
    # get coords of input point in (N,E) basis
    n = cos_phi * v[2] - sin_phi * (sin_theta * v[1] + cos_theta * v[0])
    e = cos_theta * v[1] - sin_theta * v[0]
    # rotate by negated major axis angle
    x = sin_ang * e + cos_ang * n
    y = cos_ang * e - sin_ang * n
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
        super(S2PtInEllipseTestCase, self).setUp()

    def _s2PtInEllipse(self, result, *args):
        stmt = "SELECT %ss2PtInEllipse(%s)" % (self._prefix, ",".join(map(dbparam, args)))
        rows = self.query(stmt)
        self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
        self.assertEqual(rows[0][0], result, stmt + " did not return " + repr(result))

    def testConstArgs(self):
        """Test with constant arguments.
        """
        for i in range(7):
            a = [0.0]*7; a[i] = None
            self._s2PtInEllipse(0, *a)
        for d in (-91.0, 91.0):
            self._s2PtInEllipse(None, 0.0, d, 0.0, 0.0, 0.0, 0.0, 0.0)
            self._s2PtInEllipse(None, 0.0, 0.0, 0.0, d, 0.0, 0.0, 0.0)
        self._s2PtInEllipse(None, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0)
        self._s2PtInEllipse(None, 0.0, 0.0, 0.0, 0.0, 2.0, -1.0, 0.0)
        self._s2PtInEllipse(None, 0.0, 0.0, 0.0, 0.0, 36001.0, 1.0, 0.0)
        for i in range(10):
            ra_cen = random.uniform(0.0, 360.0)
            dec_cen = random.uniform(-90.0, 90.0)
            smaa = random.uniform(0.0001, 36000.0)
            smia = random.uniform(0.00001, smaa)
            ang = random.uniform(-180.0, 180.0)
            for j in range(100):
                smaa_deg = smaa / 3600.0
                delta = smaa_deg / math.cos(math.radians(dec_cen))
                ra = random.uniform(ra_cen - delta, ra_cen + delta)
                dec = random.uniform(max(dec_cen - smaa_deg, -90.0),
                                     min(dec_cen + smaa_deg, 90.0))
                r = s2PtInEllipse(ra, dec, ra_cen, dec_cen, smaa, smia, ang)
                if r is True:
                    self._s2PtInEllipse(1, ra, dec, ra_cen, dec_cen, smaa, smia, ang)
                elif r is False:
                    self._s2PtInEllipse(0, ra, dec, ra_cen, dec_cen, smaa, smia, ang)

    def testColumnArgs(self):
        """Test with argument taken from a table.
        """
        with self.tempTable("S2PtInEllipse", ("inside INTEGER",
                                              "ra DOUBLE PRECISION",
                                              "decl DOUBLE PRECISION",
                                              "cenRa DOUBLE PRECISION",
                                              "cenDecl DOUBLE PRECISION",
                                              "smaa DOUBLE PRECISION",
                                              "smia DOUBLE PRECISION",
                                              "posAng DOUBLE PRECISION")) as t:
            for i in range(1000):
                ra_cen = random.uniform(0.0, 360.0)
                dec_cen = random.uniform(-90.0, 90.0)
                smaa = random.uniform(0.0001, 36000.0)
                smia = random.uniform(0.00001, smaa)
                ang = random.uniform(-180.0, 180.0)
                smaa_deg = smaa / 3600.0
                delta = smaa_deg / math.cos(math.radians(dec_cen))
                ra = random.uniform(ra_cen - delta, ra_cen + delta)
                dec = random.uniform(max(dec_cen - smaa_deg, -90.0),
                                     min(dec_cen + smaa_deg, 90.0))
                r = s2PtInEllipse(ra, dec, ra_cen, dec_cen, smaa, smia, ang)
                if r is True:
                    t.insert((1, ra, dec, ra_cen, dec_cen, smaa, smia, ang))
                elif r is False:
                    t.insert((0, ra, dec, ra_cen, dec_cen, smaa, smia, ang))
            stmt = """SELECT COUNT(*) FROM S2PtInEllipse
                      WHERE inside != %ss2PtInEllipse(
                          ra, decl, cenRa, cenDecl, smaa, smia, posAng)""" % self._prefix
            rows = self.query(stmt)
            self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
            self.assertEqual(rows[0][0], 0, "%s detected %d disagreements" % (stmt, rows[0][0]))


if __name__ == "__main__":
    suite = unittest.makeSuite(S2PtInEllipseTestCase)
    runner = unittest.TextTestRunner()
    if not runner.run(suite).wasSuccessful():
        sys.exit(1)

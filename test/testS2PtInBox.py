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

import sys
import unittest

from base import *


class S2PtInBoxTestCase(MySqlUdfTestCase):
    """s2PtInBox() UDF test-case. Test with constant arguments only,
    since the implementation does not perform caching of any sort.
    """
    def _s2PtInBox(self, result, *args):
        stmt = "SELECT %ss2PtInBox(%s)" % (self._prefix, ",".join(map(dbparam, args)))
        rows = self.query(stmt)
        self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
        self.assertEqual(rows[0][0], result, stmt + " did not return " + repr(result))

    def testConstArgs(self):
        """Test UDF with constant arguments.
        """
        for i in range(6):
            a = [0.0]*6
            a[i] = None
            self._s2PtInBox(0, *a)
        for d in (-91.0, 91.0):
            for i in (1, 3, 5):
                a = [0.0]*6
                a[i] = d
                self._s2PtInBox(None, *a)
        for ra_min, ra_max in ((370.0, 10.0), (50.0, -90.0), (400.0, -400.0)):
            self._s2PtInBox(None, 0.0, 0.0, ra_min, 0.0, ra_max, 0.0)
        for ra, dec in ((360.0, 0.5), (720.0, 0.5), (5.0, 0.5), (355.0, 0.5)):
            self._s2PtInBox(1, ra, dec, 350.0, 0.0, 370.0, 1.0)
        for ra, dec in ((0.0, 1.1), (0.0, -0.1), (10.1, 0.5), (349.9, 0.5)):
            self._s2PtInBox(0, ra, dec, 350.0, 0.0, 370.0, 1.0)


if __name__ == "__main__":
    suite = unittest.makeSuite(S2PtInBoxTestCase)
    runner = unittest.TextTestRunner()
    if not runner.run(suite).wasSuccessful():
        sys.exit(1)

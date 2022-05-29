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
#     - Fritz Mueller, SLAC National Accelerator Laboratory
#
# Work on this project has been sponsored by LSST and SLAC/DOE.
#

import math
import sys
import unittest

from base import *

class PhotometryTestCase(MySqlUdfTestCase):
    """Photometry UDFs test-case.
    """
    def _photFunc(self, func, result, *args):
        stmt = "SELECT %s%s(%s)" % (self._prefix, func, ",".join(map(dbparam, args)))
        rows = self.query(stmt)
        self.assertEqual(len(rows), 1, stmt + " returned multiple rows")
        self.assertTrue(
            (rows[0][0] is result) or math.isclose(rows[0][0], result), 
            repr(rows[0][0]) + " is not close to " + repr(result))

    def testNanojanskyAb(self):
        """Test nanojansky <-> AB mag UDFs.
        """
        self._photFunc('nanojanskyToAbMag', None, 0.0)
        self._photFunc('nanojanskyToAbMag', None, -1.0)
        self._photFunc('nanojanskyToAbMag', None, None)
        self._photFunc('nanojanskyToAbMag', 31.4, 1.0) # by definition
        self._photFunc('nanojanskyToAbMag', 26.4, 100.0)
        self._photFunc('nanojanskyToAbMag', 21.4, 10000.0)
        self._photFunc('nanojanskyToAbMag', 0.0, 3630.780547701e9) # zero point ("~3631 Jy")

        self._photFunc('nanojanskyToAbMagSigma', None, None, 1.0)
        self._photFunc('nanojanskyToAbMagSigma', None, 1/math.log(10), None)
        self._photFunc('nanojanskyToAbMagSigma', None, None, None)
        self._photFunc('nanojanskyToAbMagSigma', 5.0, 1/math.log(10), 2.0)

        self._photFunc('abMagToNanojansky', None, None)
        self._photFunc('abMagToNanojansky', 1.0, 31.4) # by definition
        self._photFunc('abMagToNanojansky', 100.0, 26.4)
        self._photFunc('abMagToNanojansky', 10000.0, 21.4)
        self._photFunc('abMagToNanojansky', 3630.780547701e9, 0.0) # zero point ("~3631 Jy")

        self._photFunc('abMagToNanojanskySigma', None, None, 5/math.log(10))
        self._photFunc('abMagToNanojanskySigma', None, 31.4, None)
        self._photFunc('abMagToNanojanskySigma', None, None, None)
        self._photFunc('abMagToNanojanskySigma', 2.0, 31.4, 5/math.log(10))

    def testFluxAb(self):
        """Test cgs flux <-> AB mag UDFs.
        """
        self._photFunc('fluxToAbMag', None, 0.0)
        self._photFunc('fluxToAbMag', None, -1.0)
        self._photFunc('fluxToAbMag', None, None)
        self._photFunc('fluxToAbMag', 0.0, 3630.780547701e-23) # zero point
        self._photFunc('fluxToAbMag', -48.6, 1.0) # by definition
        self._photFunc('fluxToAbMag', -53.6, 100.0)
        self._photFunc('fluxToAbMag', -58.6, 10000.0)

        self._photFunc('fluxToAbMagSigma', None, None, 1.0)
        self._photFunc('fluxToAbMagSigma', None, 1/math.log(10), None)
        self._photFunc('fluxToAbMagSigma', None, None, None)
        self._photFunc('fluxToAbMagSigma', 5.0, 1/math.log(10), 2.0)

        self._photFunc('abMagToFlux', None, None)
        self._photFunc('abMagToFlux', 3630.780547701e-23, 0.0) # zero point
        self._photFunc('abMagToFlux', 1.0, -48.6) # by definition
        self._photFunc('abMagToFlux', 100.0, -53.6)
        self._photFunc('abMagToFlux', 10000.0, -58.6)

        self._photFunc('abMagToFluxSigma', None, None, 5/math.log(10))
        self._photFunc('abMagToFluxSigma', None, -48.6, None)
        self._photFunc('abMagToFluxSigma', None, None, None)
        self._photFunc('abMagToFluxSigma', 2.0, -48.6, 5/math.log(10))

if __name__ == "__main__":
    suite = unittest.makeSuite(PhotometryTestCase)
    runner = unittest.TextTestRunner()
    if not runner.run(suite).wasSuccessful():
        sys.exit(1)

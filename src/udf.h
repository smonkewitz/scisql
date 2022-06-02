/*
    Copyright (C) 2011-2022 the SciSQL authors

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    Authors:
        - Serge Monkewitz, IPAC/Caltech

    Work on this project has been sponsored by LSST and SLAC/DOE.

    ----------------------------------------------------------------

    Macros to help implemented unversioned UDFs in terms of their
    versioned implementations.
*/

#ifndef SCISQL_UDF_H
#define SCISQL_UDF_H

#include "common.h"

#define SCISQL_CAT2_IMPL(a,b) a ## b
#define SCISQL_CAT2(a,b) SCISQL_CAT2_IMPL(a,b)

#define SCISQL_CAT3_IMPL(a,b,c) a ## b ## c
#define SCISQL_CAT3(a,b,c) SCISQL_CAT3_IMPL(a,b,c)

#define SCISQL_STRINGIZE_IMPL(s) #s
#define SCISQL_STRINGIZE(s) SCISQL_STRINGIZE_IMPL(s)

/*  Produces a name prefixed with SCISQL_PREFIX and with the given suffix.
 */
#define SCISQL_FNAME(name, suffix) SCISQL_CAT3(SCISQL_PREFIX, name, suffix)

/*  Use as the suffix argument to SCISQL_FNAME or SCISQL_VERSIONED_FNAME to
    produce a name without a suffix.
 */
#define SCISQL_NO_SUFFIX

/*  Produces a name prefixed with SCISQL_PREFIX and suffixed with a version
    followed by the given suffix.
 */
#define SCISQL_VERSIONED_FNAME(name, suffix) \
    SCISQL_FNAME(name, SCISQL_CAT2(SCISQL_VSUFFIX, suffix))

/*  Produces a name prefixed with SCISQL_PREFIX and suffixed with a version
    as a quoted string.
 */
#define SCISQL_UDF_NAME(name) \
    SCISQL_STRINGIZE(SCISQL_VERSIONED_FNAME(name, SCISQL_NO_SUFFIX))

/*  Macros to address API type differences between MySQL 8 and MariaDB 10.
 */
#ifdef LIBMARIADB
#define SCISQL_BOOL my_bool
#else
#define SCISQL_BOOL bool
#endif

/*  Implements an unversioned init function in terms of the versioned one.
 */
#define SCISQL_UDF_INIT(name) \
    SCISQL_API SCISQL_BOOL SCISQL_FNAME(name, _init) ( \
        UDF_INIT *initid, \
        UDF_ARGS *args, \
        char *message) \
    { \
        return SCISQL_VERSIONED_FNAME(name, _init) (initid, args, message); \
    }

/*  Implements an unversioned deinit function in terms of the versioned one.
 */
#define SCISQL_UDF_DEINIT(name) \
    SCISQL_API void SCISQL_FNAME(name, _deinit) (UDF_INIT *initid) { \
        return SCISQL_VERSIONED_FNAME(name, _deinit) (initid); \
    }

/*  Implements an unversioned REAL UDF in terms of the versioned one.
 */
#define SCISQL_REAL_UDF(name) \
    SCISQL_API double SCISQL_FNAME(name, SCISQL_NO_SUFFIX) ( \
        UDF_INIT *initid, \
        UDF_ARGS *args, \
        char *is_null, \
        char *error) \
    { \
        return SCISQL_VERSIONED_FNAME(name, SCISQL_NO_SUFFIX) ( \
            initid, args, is_null, error); \
    }

/*  Implements an unversioned INTEGER UDF in terms of the versioned one.
 */
#define SCISQL_INTEGER_UDF(name) \
    SCISQL_API long long SCISQL_FNAME(name, SCISQL_NO_SUFFIX) ( \
        UDF_INIT *initid, \
        UDF_ARGS *args, \
        char *is_null, \
        char *error) \
    { \
        return SCISQL_VERSIONED_FNAME(name, SCISQL_NO_SUFFIX) ( \
            initid, args, is_null, error); \
    }

/*  Implements an unversioned STRING UDF in terms of the versioned one.
 */
#define SCISQL_STRING_UDF(name) \
    SCISQL_API char * SCISQL_FNAME(name, SCISQL_NO_SUFFIX) ( \
        UDF_INIT *initid, \
        UDF_ARGS *args, \
        char *result, \
        unsigned long *length, \
        char *is_null, \
        char *error) \
    { \
        return SCISQL_VERSIONED_FNAME(name, SCISQL_NO_SUFFIX) ( \
            initid, args, result, length, is_null, error); \
    }

/*  Implements an unversioned clear function in terms of the versioned one.
 */
#define SCISQL_UDF_CLEAR(name) \
    SCISQL_API void SCISQL_FNAME(name, _clear) ( \
        UDF_INIT *initid, \
        char *is_null, \
        char *error) \
    { \
        SCISQL_VERSIONED_FNAME(name, _clear) (initid, is_null, error); \
    }

/*  Implements an unversioned add function in terms of the versioned one.
 */
#define SCISQL_UDF_ADD(name) \
    SCISQL_API void SCISQL_FNAME(name, _add) ( \
        UDF_INIT *initid, \
        UDF_ARGS *args, \
        char *is_null, \
        char *error) \
    { \
        SCISQL_VERSIONED_FNAME(name, _add) (initid, args, is_null, error); \
    }

/*  Implements an unversioned reset function in terms of the versioned one.
 */
#define SCISQL_UDF_RESET(name) \
    SCISQL_API void SCISQL_FNAME(name, _reset) ( \
        UDF_INIT *initid, \
        UDF_ARGS *args, \
        char *is_null, \
        char *error) \
    { \
        SCISQL_VERSIONED_FNAME(name, _reset) (initid, args, is_null, error); \
    }

#endif /* SCISQL_UDF_H */

/*
    Copyright (C) 2011 Serge Monkewitz

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
*/
#include <stdio.h>
#include <stdlib.h>

#include "select.h"


#define SCISQL_ASSERT_EQUAL(a, b, ...) \
    do { \
        if ((a) != (b)) { \
            fprintf(stderr, #a " != " #b " : " __VA_ARGS__); \
            exit(1); \
        } \
    } while(0)


#define SCISQL_ASSERT_NOT_EQUAL(a, b, ...) \
    do { \
        if ((a) == (b)) { \
            fprintf(stderr, #a " != " #b " : " __VA_ARGS__); \
            exit(1); \
        } \
    } while(0)


static size_t uniform(size_t n, unsigned short seed[3]) {
    return (size_t) (n * erand48(seed));
}


static void shuffle(double *array, size_t n, unsigned short seed[3]) {
    size_t i, j, k;
    double tmp;
    for (i = 0; i < n; ++i) {
        j = uniform(n, seed);
        k = uniform(n, seed);
        tmp = array[j];
        array[j] = array[k];
        array[k] = tmp;
    }
}


typedef struct {
    size_t n;
    size_t nperms;
    double array[10];
} permgen;


static size_t factorial(size_t n) {
    size_t f = n;
    for (--n; n > 1; --n) {
        f *= n;
    }
    return f;
}


static void permgen_init(permgen *gen, size_t n) {
    size_t i;

    if (n == 0 || gen == 0 || n > 10) {
        fprintf(stderr, "Invalid arguments to permutation generator "
                "initialization function\n");
        exit(1);
    }
    gen->n = n;
    gen->nperms = factorial(n);
    for (i = 0; i < n; ++i) {
        gen->array[i] = i;
    }
}


static void permgen_next(permgen *gen) {
    double tmp;
    size_t j, k;

    for (j = gen->n - 2; gen->array[j] > gen->array[j + 1]; --j) {
        if (j == 0) {
            return; /* last permutation already generated */
        }
    }
    for (k = gen->n - 1; gen->array[j] > gen->array[k]; --k) { }
    tmp = gen->array[k];
    gen->array[k] = gen->array[j];
    gen->array[j] = tmp;
    for (j = j + 1, k = gen->n - 1; j < k; ++j, --k) {
        tmp = gen->array[k];
        gen->array[k] = gen->array[j];
        gen->array[j] = tmp;
    }
}


static void test(double (*select)(double *, size_t, size_t)) {
    static const size_t MAX_N = 1024*1024;

    double *array;
    double expected, actual;
    size_t n;
    unsigned short seed[3] = { 10, 20, 30 };

    array = (double *) malloc(MAX_N*sizeof(double));
    SCISQL_ASSERT_NOT_EQUAL(array, 0, "memory allocation failed");

    /* Test all possible permutations of n distinct values, for n = 1..9 */
    for (n = 1; n <= 10; ++n) {
        permgen gen;
        size_t i;

        permgen_init(&gen, n);
        expected = (n >> 1);
        for (i = 0; i < gen.nperms; ++i, permgen_next(&gen)) {
            memcpy(array, gen.array, n * sizeof(double));
            actual = (*select)(array, n, n >> 1);
            SCISQL_ASSERT_EQUAL(expected, actual, "median failed on permutaion "
                "%llu of %llu permutations of 0 .. %d", (unsigned long long) i,
                (unsigned long long) gen.nperms, (int) n - 1);
        }
    }

    /* Test sequences of identical values */
    for (n = 1; n < 100; ++n) {
        size_t i;
        for (i = 0; i < n; ++i) {
            array[i] = 1.0;
        }
        expected = 1.0;
        actual = (*select)(array, n, n >> 2);
        SCISQL_ASSERT_EQUAL(expected, actual, "quartile failed on array of "
            "%llu identical values", (unsigned long long) n);
    }

    /* Test sequence of ascending values */
    for (n = 1; n <= MAX_N; n *= 2) {
        size_t i;
        for (i = 0; i < n; ++i) {
            array[i] = i;
        }
        expected = n >> 1;
        actual = (*select)(array, n, n >> 1);
        SCISQL_ASSERT_EQUAL(expected, actual, "median failed on array of "
            "%llu ascending values", (unsigned long long) n);
    }

    /* Test sequence of descending values */
    for (n = 1; n <= MAX_N; n *= 2) {
        size_t i;
        for (i = 0; i < n; ++i) {
            array[i] = n - 1 - i;
        }
        expected = n >> 1;
        actual = (*select)(array, n, n >> 1);
        SCISQL_ASSERT_EQUAL(expected, actual, "median failed on array of "
            "%llu descending values", (unsigned long long) n);
    }

    /* Test randomly shuffled sequences of values */
    for (n = 1; n <= MAX_N; n *= 2) {
        size_t i;
        for (i = 0; i < n; ++i) {
            array[i] = n - 1 - i;
        }
        shuffle(array, n, seed);
        expected = n >> 1;
        actual = (*select)(array, n, n >> 1);
        SCISQL_ASSERT_EQUAL(expected, actual, "median failed on array of "
            "%llu shuffled distinct values", (unsigned long long) n);
    }

    /* Test sequences containing duplicate values */
    for (n = 1; n <= MAX_N; n = 5*n/4 + 1) {
        size_t i, j;
        for (i = 0, j = 0; i < n; ++i) {
            array[i] = j;
            if (erand48(seed) > 0.7) {
                ++j;
            }
        }
        expected = array[n >> 1];
        shuffle(array, n, seed);
        actual = (*select)(array, n, n >> 1);
        SCISQL_ASSERT_EQUAL(expected, actual, "median failed on array of "
            "%llu shuffled values with duplicates", (unsigned long long) n);
     }
}


int main(int argc SCISQL_UNUSED, char **argv SCISQL_UNUSED) {
    test(&scisql_select);
    test(&scisql_selectmm);
    return 0;
}


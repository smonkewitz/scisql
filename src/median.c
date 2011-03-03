
// Use and redistribution of this software is covered by the GNU Public License 
// Version 3 (GPLv3) or later, as detailed below. A copy of the GPLv3 is also 
// available at <http://www.gnu.org/licenses/>.

// The "median" user defined function for MySQL. Compatible with mysql 5.x
//
// Input parameters:
//  * the values (double)
//
// Output:
//  * median value of the distribution of the values (double)
//
// Authors:
//  * Jacek Becla <becla@slac.stanford.edu>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "mysql/mysql.h"


// The function uses malloc to keep the first 8K values.
// In case there are more than 8K values, it transparently
// switches to mmap file (in a /tmp directory). (In the future
// versions we might consider using mysql's TMP_DIR instead)
// At the moment, it can handle up to ~134 million values
// (see MAX_NELEMS below).


struct
MedianStruct {
    int64_t nValues;   // current number of values stored
    double *values;    // the values stored
    int mmapFDescr;    // file descr, or 0 if malloc used
};
const int N_SLOTS_4MALLOC = 8192; // use malloc for the first 8K elements (64KB)
char tempFName[32] = "/tmp/mysql.median.XXXXXX";
const int64_t MMAP_FSIZE = 1ULL << 30;  // 1GB
const int64_t MAX_NELEMS = (1ULL << 30) / sizeof(double);

char errMsg[MYSQL_ERRMSG_SIZE] = "";


void median_freeValues(struct MedianStruct*);
my_bool median_init(UDF_INIT*, UDF_ARGS*, char*);
void median_deinit(UDF_INIT*);
void median_clear(UDF_INIT *, char *, char *);
void median_reset(UDF_INIT*, UDF_ARGS*, char*, char*);
void median_add(UDF_INIT*, UDF_ARGS*, char*, char*);
double median(UDF_INIT*, UDF_ARGS*, char*, char*);


void
median_freeValues(struct MedianStruct* buffer) {
    if ( buffer->values == NULL ) {
        return;
    }
    if ( buffer->mmapFDescr == 0 ) {
        free(buffer->values);
    } else {
        close(buffer->mmapFDescr);
        unlink(tempFName);
        buffer->mmapFDescr = 0;
    }
    buffer->values = NULL;
}


my_bool
median_init(UDF_INIT* initId, UDF_ARGS* args, char* message) {
    if ( args->arg_count != 1 ) {
        strncpy(message, "1 argument expected", MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    if ( args->arg_type[0] != REAL_RESULT ) {
        strncpy(message, "argument should be DOUBLE", MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    if ( strcmp(errMsg, "") ) { 
        strncpy(message, errMsg, MYSQL_ERRMSG_SIZE - 1);
        message[MYSQL_ERRMSG_SIZE - 1] = '\0';
        return 1;
    }
    struct MedianStruct *buffer =
        (struct MedianStruct*) malloc(sizeof(struct MedianStruct));
    buffer->nValues = 0;
    buffer->values = NULL;
    buffer->mmapFDescr = 0;

    initId->maybe_null = 1;
    initId->max_length = 32;
    initId->ptr = (char*) buffer;

    return 0;
}


void
median_deinit(UDF_INIT* initId) {
    struct MedianStruct *buffer = (struct MedianStruct*) initId->ptr;
    median_freeValues(buffer);
    free(initId->ptr);
    initId->ptr = NULL;
}


void
median_clear(UDF_INIT *initId, char *isNull, char *error) {
    struct MedianStruct *buffer = (struct MedianStruct*) initId->ptr;
    buffer->nValues = 0;
    *isNull = 0;
    *error = 0;
    median_freeValues(buffer);
    buffer->values = (double *) malloc(N_SLOTS_4MALLOC*sizeof(double));
}


void
median_reset(UDF_INIT* initId, UDF_ARGS* args, char* isNull, char* error) {
    median_clear(initId, isNull, error);
    median_add(initId, args, isNull, error);
}


void
median_add(UDF_INIT* initId, UDF_ARGS* args, char* isNull, char* error) {
    if ( args->args[0] == NULL ) {
        return;
    }
    struct MedianStruct *buffer = (struct MedianStruct*) initId->ptr;
    if ( buffer->nValues >= MAX_NELEMS ) {
        *error = 1;
        strncpy(errMsg,
                "too many rows (out of space)",
                MYSQL_ERRMSG_SIZE - 1);
        return;
    }
    if ( buffer->nValues == N_SLOTS_4MALLOC ) {
        // create temporary sparse file
        int fd = mkstemp(tempFName);
        ftruncate(fd, MMAP_FSIZE);
        // memory map it
        double* dPtr = mmap(NULL,
                            MMAP_FSIZE,
                            PROT_READ | PROT_WRITE, MAP_SHARED,
                            fd,
                            0);
        if ( dPtr == MAP_FAILED ) {
            *error = 1;
            strncpy(errMsg,
                    "too many rows (out of space in mmap)",
                    MYSQL_ERRMSG_SIZE - 1);
            return;
        }
        // copy data to the memory mapped file, clean up
        memcpy(dPtr, buffer->values, N_SLOTS_4MALLOC*sizeof(double));
        free(buffer->values);
        buffer->values = dPtr;
        buffer->mmapFDescr = fd;
    }
    buffer->values[buffer->nValues++] = *((double*)args->args[0]);
}


int
compare_doubles(const void *d1, const void *d2) {
    const double *dd1 = (const double *) d1;
    const double *dd2 = (const double *) d2;
    return (*dd1 > *dd2) - (*dd1 < *dd2);
}


double
median(UDF_INIT* initId, UDF_ARGS* args, char* isNull, char* error) {
    struct MedianStruct* buffer = (struct MedianStruct*) initId->ptr;

    if (buffer->nValues == 0 || *error != 0) {
        *isNull = 1;
        return 0.0;
    }
    *isNull = 0;
    if (buffer->nValues == 1) {
        return buffer->values[0];
    }
    qsort(buffer->values, buffer->nValues, sizeof(double), compare_doubles);

    if ( (buffer->nValues&1) == 1 ) {
        return buffer->values[(buffer->nValues-1)/2];
    } else {
        return (buffer->values[(buffer->nValues-2)/2]+
                buffer->values[buffer->nValues/2])/2.0;
    }
}

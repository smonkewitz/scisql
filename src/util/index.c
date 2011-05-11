/*
    Copyright (C) 2011 Serge Monkewitz

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License v3 as published
    by the Free Software Foundation, or any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    A copy of the LGPLv3 is available at <http://www.gnu.org/licenses/>.

    Authors:
        - Serge Monkewitz, IPAC/Caltech

    Work on this project has been sponsored by LSST and SLAC/DOE.

    ----------------------------------------------------------------

    Simple utility for indexing TSV tables of points, circles, or polygons
    on the shere.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "htm.h"

#ifdef __cplusplus
extern "C" {
#endif


/*  Options and processing context.
 */
typedef struct {
    long nskip;       /* Number of initial lines to skip */
    int ranges;       /* Output ID ranges instead of IDs */
    int verbose;      /* Verbose output? */
    int ncols;        /* number of columns expected per-row */
    int level;        /* subdivision level */
    size_t maxranges; /* maximum number of ranges to output per region */
} _scisql_context;


/*  Describes correct usage.
 */
static void usage(const char *name) {
    fprintf(stderr, "Usage: %s [options] out_file in_file_1 [in_file_2 ...]\n\n",
            name);
    fprintf(stderr,
        "Indexes a tab-separated table containing a single ID column followed\n"
        "by a trailing list of spatial columns. The number of spatial columns\n"
        "determines how they are interpreted:\n"
        "\n"
        "\t3:   the last 3 columns are treated as circle center\n"
        "\t     longitude, center latitude and circle radius,\n"
        "\t     all in degrees.\n"
        "\t2*N: the last 2*N (N >= 3) columns are treated as a\n"
        "\t     list of longitude, latitude polygon vertex\n"
        "\t     coordinates, all in degrees.\n"
        "\n"
        "The number of columns must be consistent for every row in the table,\n"
        "or an error is signalled. Specifying \"-\" as the outut file name\n"
        "will cause output to be written to standard out.\n"
        "\n"
        "    The TSV parser is currently very simplistic:\n"
        "\t- fields must be separated by '\\t'\n"
        "\t- lines must be terminated by '\\n'\n"
        "\t- quoted strings are not recognized\n"
        "\t- character escapes are not recognized, and their\n"
        "\t  presence in spatial columns results in an error. In\n"
        "\t  particular, '\\N' (NULL) values will result in an\n"
        "\t  error.\n"
        "\n"
        "Each input ID is output multiple times: once for each HTM ID or\n"
        "range of HTM IDs overlapping the corresponding circle/polygon.\n"
        "The HTM ID or ID range is appended in one or two trailing\n"
        "integer-valued columns.\n"
        "\n"
        "Options\n"
        "\t-i <type>  Specifies the spatial index type to use;\n"
        "\t           for now, only \"htm\" is supported. This\n"
        "\t           is the default.\n"
        "\t-l <level> The subdivision level to use when indexing;\n"
        "\t           the default is 10.\n"
        "\t-r         Output ID ranges rather than IDs.\n"
        "\t-m <N>     Bound on the maximum number of HTM ID\n"
        "\t           ranges generated for a region. Note that with\n"
        "\t           arbitrary input geometry, up to 4 ranges may\n"
        "\t           be generated no matter what the subdivision\n"
        "\t           level is. So for N < 4, the requested bound\n"
        "\t           may not be achieved.\n"
        "\t-s <N>     Skip the first N lines in each input\n"
        "\t           file.\n"
        "\t-v         Chatty progress messages.\n"
        "\n");
    fflush(stderr);
}


/*  Returns a pointer to the character following the first occurence of sep
    in the string beg, or end if no occurrence is found.
 */
SCISQL_INLINE const char * advance(const char *beg, const char *end, char sep) {
    for (; beg < end && *beg != sep; ++beg) { }
    return (beg < end) ? beg + 1 : end;
}


/*  Outputs the IDs or ID ranges computed for an input.
 */
static int output_ids(_scisql_context *ctx,
                      const scisql_ids *ids,
                      const char *beg,
                      const char *end,
                      FILE *out)
{
    char buf[128];
    if (ctx->ranges != 0) {
        size_t i;
        int nc;
        for (i = 0; i < ids->n; ++i) {
            if (fwrite(beg, end - beg, 1, out) != 1) {
                return 1;
            }
            nc = snprintf(buf, sizeof(buf), "%lld\t%lld\n",
                          (long long) ids->ranges[2*i],
                          (long long) ids->ranges[2*i + 1]);
            if (nc < 0 || nc >= (int) sizeof(buf)) {
                return 1;
            }
            if (fwrite(buf, (size_t) nc, 1, out) != 1) {
                return 1;
            }
        }
    } else {
        size_t i;
        long long id;
        int nc;
        for (i = 0; i < ids->n; ++i) {
            for (id = ids->ranges[2*i]; id <= ids->ranges[2*i + 1]; ++id) {
                if (fwrite(beg, end - beg, 1, out) != 1) {
                    return 1;
                }
                nc = snprintf(buf, sizeof(buf), "%lld\n", id);
                if (nc < 0 || nc >= (int) sizeof(buf)) {
                    return 1;
                }
                if (fwrite(buf, (size_t) nc, 1, out) != 1) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

static double get_double(const char **msg,
                         const char *beg,
                         const char *end,
                         int last)
{
    char *endptr;
    double d;
    const char *e = end;

    for (; beg < e && *beg == ' '; ++beg) { }
    if (beg == e || isspace(*beg)) {
        *msg = "empty field";
        return 0.0;
    }
    if (e[-1] == '\t' || e[-1] == '\n') {
        --e;
    }
    for (; e > beg && e[-1] == ' '; --e) { }
    if (last == 0 || e != end) {
        d = strtod(beg, &endptr);
    } else {
        /* The very last column of the last row might not be terminated
           with '\n'. If its last character is the last byte of a page,
           then strtod would walk off then end of our memory map and
           seg-fault. */
        size_t n = (size_t) (e - beg);
        char *s = (char *) malloc(n + 1);
        if (s == 0) {
            *msg = "memory allocation failed";
            return 0.0;
        }
        memcpy(s, beg, n);
        s[n] = '\0';
        e = s + n;
        d = strtod(s, &endptr);
        free(s);
    }
    if (endptr != e) {
        *msg = "invalid floating point number in field";
        return 0.0;
    }
    return d;
}

/*  Indexes a table of spherical circles.
 */
static int index_s2circle(_scisql_context *ctx,
                          const char *file,
                          const char *beg,
                          const char *end,
                          FILE *out)
{
    double lon, lat, radius;
    scisql_sc p;
    scisql_v3 center;
    scisql_ids *ids = 0;
    const char *msg = 0;
    long long line = ctx->nskip;

    if (ctx->verbose != 0) {
        fprintf(stderr, "Indexing file %s (spherical circles)\n", file);
        fflush(stderr);
    }

    while (beg < end) {
        const char *sid = beg;
        const char *eol = advance(beg, end, '\n');
        const char *slon = advance(sid, eol, '\t');
        const char *slat = advance(slon, eol, '\t');
        const char *sradius = advance(slat, eol, '\t');
        if (slon == eol || slat == eol || sradius == eol ||
            advance(sradius, eol, '\t') != eol) {
            msg = "invalid row - expecting id lon lat radius";
            goto fail_msg;
        }
        lon = get_double(&msg, slon, slat, 0);
        if (msg != 0) {
            goto fail_msg;
        }
        lat = get_double(&msg, slat, sradius, 0);
        if (msg != 0) {
            goto fail_msg;
        }
        radius = get_double(&msg, sradius, eol, eol == end);
        if (msg != 0) {
            goto fail_msg;
        }
        if (scisql_sc_init(&p, lon, lat) != 0) {
            msg = "invalid circle center longitude/latitude (columns 2,3)";
            goto fail_msg;
        }
        if (radius < 0.0 || SCISQL_ISNAN(radius)) {
            msg = "invalid circle radius (column 4)";
            goto fail_msg;
        }
        scisql_sctov3(&center, &p);
        ids = scisql_s2circle_htmids(ids, &center, radius,
                                     ctx->level, ctx->maxranges);
        if (ids == 0) {
            msg = "failed to index circle";
            goto fail_msg;
        }
        if (output_ids(ctx, ids, sid, slon, out) != 0) {
            msg = "failed to output indexes overlapping circle";
            goto fail_msg;
        }
        ++line;
        beg = eol;
    }
    return 0;
fail_msg:
    fprintf(stderr, "ERROR [%s:%lld]: %s\n", file, line, msg);
    return 1;
}

/*  Indexes a table of spherical convex polygons.
 */
static int index_s2cpoly(_scisql_context *ctx,
                         const char *file,
                         const char *beg,
                         const char *end,
                         FILE *out)
{
    double lon, lat;
    scisql_sc p;
    scisql_v3 verts[SCISQL_MAX_VERTS];
    scisql_s2cpoly poly;
    scisql_ids *ids = 0;
    const char *msg = 0;
    long long line = ctx->nskip;
    int nv = (ctx->ncols - 1) / 2;

    if (ctx->verbose != 0) {
        fprintf(stderr, "Indexing file %s (spherical convex polygons)\n", file);
        fflush(stderr);
    }

    while (beg < end) {
        const char *sid = beg;
        const char *eol = advance(beg, end, '\n');
        const char *slon = advance(sid, eol, '\t');
        const char *sidend = slon;
        int i;
        for (i = 0; i < nv; ++i) {
            const char *slat = advance(slon, eol, '\t');
            if (slat == eol) {
                msg = "invalid row - expecting "
                      "id lon1 lat1 lon2 lat2...";
                goto fail_msg;
            }
            lon = get_double(&msg, slon, slat, 0);
            if (msg != 0) {
                goto fail_msg;
            }
            slon = advance(slat, eol, '\t');
            lat = get_double(&msg, slat, slon, slon == eol);
            if (msg != 0) {
                goto fail_msg;
            }
            if (scisql_sc_init(&p, lon, lat) != 0) {
                msg = "invalid vertex longitude/latitude";
                goto fail_msg;
            }
            scisql_sctov3(&verts[i], &p);
        }
        if (slon != eol) {
            msg = "invalid row - expecting "
                  "id lon1 lat1 lon2 lat2...";
            goto fail_msg;
        }
        if (scisql_s2cpoly_init(&poly, verts, nv) != 0) {
            msg = "invalid polygon";
            goto fail_msg;
        }
        ids = scisql_s2cpoly_htmids(ids, &poly, ctx->level, ctx->maxranges);
        if (ids == 0) {
            msg = "failed to index polygon";
            goto fail_msg;
        }
        if (output_ids(ctx, ids, sid, sidend, out) != 0) {
            msg = "failed to output indexes overlapping polygon";
            goto fail_msg;
        }
        ++line;
        beg = eol;
    }
    return 0;
fail_msg:
    fprintf(stderr, "ERROR [%s:%lld]: %s\n", file, line, msg);
    return 1;
}


/* Dispatches to correct indexing function based on column count.
 */
static int index_dispatch(_scisql_context *ctx,
                          const char *file,
                          const char *beg,
                          const char *end,
                          FILE *out)
{
    long n;
    /* skip requested number of initial rows */
    for (n = ctx->nskip; n > 0; --n) {
        beg = advance(beg, end, '\n');
        if (beg >= end) {
            /* no data left */
            if (ctx->verbose != 0) {
                fprintf(stderr, "Skipping file %s (no records)\n", file);
                fflush(stderr);
            }
            return 0;
        }
    }
    if (ctx->ncols == 0) {
        /* parse first row to determine number of columns */
        const char *field = beg;
        const char *eol = advance(beg, end, '\n');
        int ncols = 0;
        for (; field < eol; field = advance(field, eol, '\t')) {
            ++ncols;
        }
        if (ncols < 4 || ncols == 5 || ncols == 6 ||
            (ncols > 6 && (ncols & 1) == 0)) {
            fprintf(stderr, "ERROR: line %ld in file %s has an invalid "
                    "number of columns\n", ctx->nskip, file);
            return 1;
        }
        ctx->ncols = ncols;
    }
    if (ctx->ncols == 4) {
        /* circles */
        return index_s2circle(ctx, file, beg, end, out);
    }
    /* polygons */
    return index_s2cpoly(ctx, file, beg, end, out);
}


/*  Driver routine for indexing a TSV file of regions.
 */
static int index_file(_scisql_context *ctx, const char *file, FILE *out) {
    struct stat buf;
    size_t nbytes;
    const char *data;
    int fd, ret, prot, flgs;

    /* open input file and determine its size */
    fd = open(file, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "ERROR: failed to open file %s for reading\n", file);
        return 1;
    }
    if (fstat(fd, &buf) != 0) {
        fprintf(stderr, "ERROR: failed to stat file %s\n", file);
        close(fd);
        return 1;
    }
    nbytes = (size_t) buf.st_size;
    if (nbytes == 0) {
        /* empty file - nothing to do */
        if (ctx->verbose != 0) {
            fprintf(stderr, "Skipping file %s (empty)\n", file);
            fflush(stderr);
        }
        close(fd);
        return 0;
    }
    /* memory map the input file */
    prot = PROT_READ;
    flgs = MAP_SHARED;
#if MAP_HUGETLB
    if (nbytes / 64 > (size_t) getpagesize()) {
        flgs |= MAP_HUGETLB;
    }
#endif
    data = (const char *) mmap(0, nbytes, prot, flgs, fd, 0);
    if (data == MAP_FAILED) {
#if MAP_HUGETLB
        if (flgs != MAP_SHARED) {
            /* try again without huge pages */
            data = (const char *) mmap(0, nbytes, prot, MAP_SHARED, fd, 0);
            if (data == MAP_FAILED) {
#endif
                fprintf(stderr, "ERROR: failed to memory map file %s\n", file);
                close(fd);
                return 1;
#if MAP_HUGETLB
            }
        }
#endif
    }
    /* advise the kernel that we will perform one sequential pass
       through the file. */
    madvise((void *) data, nbytes, MADV_SEQUENTIAL);
    /* dispatch to indexing function */
    ret = index_dispatch(ctx, file, data, data + nbytes, out);
    /* resource cleanup */
    munmap((void *) data, nbytes);
    close(fd);
    return ret;
}


/* ---- Entry point ---- */

int main(int argc, char **argv) {
    _scisql_context ctx;
    FILE *out = 0;
    char *end = 0;
    long l;
    int c, i;

    memset(&ctx, 0, sizeof(_scisql_context));
    ctx.level = 10;
    ctx.maxranges = SIZE_MAX;

    /* parse command line arguments */
    opterr = 0;
    while ((c = getopt(argc, argv, "i:l:m:rs:v")) != -1) {
        switch(c) {
            case 'i':
                if (optarg == 0 || strcmp(optarg, "htm") != 0) {
                    fprintf(stderr, "ERROR: the only supported option value "
                            "for -%c is currently \"htm\"\n", optopt);
                    return 1;
                }
                break;
            case 'l':
                if (optarg == 0) {
                    fprintf(stderr, "ERROR: option -%c requires an argument\n",
                            optopt);
                    return 1;
                }
                l = strtol(optarg, &end, 0);
                if (end == 0 || end == optarg) {
                    fprintf(stderr, "ERROR: option -%c requires an integer "
                            "argument in range [0,%d]\n", optopt,
                            SCISQL_HTM_MAX_LEVEL);
                    return 1;
                }
                if (l < 0 || l > SCISQL_HTM_MAX_LEVEL) {
                    fprintf(stderr, "ERROR: option -%c requires an integer "
                            "argument in range [0,%d]\n", optopt,
                            SCISQL_HTM_MAX_LEVEL);
                    return 1;
                }
                ctx.level = (int) l;
                break;
            case 'm':
                if (optarg == 0) {
                    fprintf(stderr, "ERROR: option -%c requires an argument\n",
                            optopt);
                    return 1;
                }
                ctx.maxranges = (size_t) strtoull(optarg, &end, 0);
                if (end == 0 || end == optarg) {
                    fprintf(stderr, "ERROR: option -%c requires a non-negative "
                            "integer argument\n", optopt);
                    return 1;
                }
                break;
            case 'r':
                ctx.ranges = 1;
                break;
            case 's':
                if (optarg == 0) {
                    fprintf(stderr, "ERROR: option -%c requires an argument\n",
                            optopt);
                    return 1;
                }
                ctx.nskip = strtol(optarg, &end, 0);
                if (end == 0 || end == optarg) {
                    fprintf(stderr, "ERROR: option -%c requires an integer "
                            "argument\n", optopt);
                    return 1;
                }
                break;
            case 'v':
                ctx.verbose = 1;
                break;
            case '?':
                if (optopt == 'i' || optopt == 'l' ||
                    optopt == 'm' || optopt == 's') {
                    fprintf(stderr, "ERROR: option -%c requires an argument\n",
                            optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "ERROR: unknown option -%c\n", optopt);
                } else {
                    fprintf(stderr, "ERROR: unknown option character \\x%x\n",
                            optopt);
                }
                return 1;
            default:
                usage(argv[0]);
                return 1;
        }
    }
    if (ctx.ranges == 0) {
        ctx.maxranges = SIZE_MAX;
    }
    if (argc - optind < 2) {
        usage(argv[0]);
        return 1;
    }
    if (strcmp(argv[optind], "-") == 0) {
        out = stdout;
    } else {
        out = fopen(argv[optind], "wb");
        if (out == 0) {
            fprintf(stderr, "ERROR: failed to open output file %s "
                    "for writing\n", argv[optind]);
            return 1;
        }
    }
    for (i = optind + 1; i < argc; ++i) {
        if (index_file(&ctx, argv[i], out) != 0) {
            fclose(out);
            return 1;
        }
    }
    if (fclose(out) != 0) {
        fprintf(stderr, "ERROR: failed to close output stream\n");
        return 1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif


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

    Minimalistic HTM library implementation.

    This software is based on work by A. Szalay, T. Budavari,
    G. Fekete at The Johns Hopkins University, and Jim Gray,
    Microsoft Research. See the following links for more information:

    http://voservices.net/spherical/
    http://adsabs.harvard.edu/abs/2010PASP..122.1375B
 */
#include "htm.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
    HTM triangles are subdivided into 4 sub-triangles as follows :

            v2
             *
            / \
           /   \
      sv1 *-----* sv0
         / \   / \
        /   \ /   \
    v0 *-----*-----* v1
            sv2

     -  vertices are unit magnitude 3-vectors
     -  edges are great circles on the unit sphere
     -  vertices are stored in counter-clockwise order
       (when viewed from outside the unit sphere in a
       right handed coordinate system)
     -  sv0 = (v1 + v2) / ||v1 + v2||, and likewise for sv1, sv2

    Note that if the HTM triangle given by (v0,v1,v2) has index I, then:
     -  sub triangle T0 = (v0,sv2,sv1) has index I*4
     -  sub triangle T1 = (v1,sv0,sv2) has index I*4 + 1
     -  sub triangle T2 = (v2,sv1,sv0) has index I*4 + 2
     -  sub triangle T3 = (sv0,sv1,sv2) has index I*4 + 3

    All HTM triangles are obtained via subdivision of 8 initial
    triangles, defined from the following set of 6 vertices :
     -  V0 = ( 0,  0,  1) north pole
     -  V1 = ( 1,  0,  0)
     -  V2 = ( 0,  1,  0)
     -  V3 = (-1,  0,  0)
     -  V4 = ( 0, -1,  0)
     -  V5 = ( 0,  0, -1) south pole

    The root triangles (corresponding to subdivision level 0) are :
     -  S0 = (V1, V5, V2), HTM index = 8
     -  S1 = (V2, V5, V3), HTM index = 9
     -  S2 = (V3, V5, V4), HTM index = 10
     -  S3 = (V4, V5, V1), HTM index = 11
     -  N0 = (V1, V0, V4), HTM index = 12
     -  N1 = (V4, V0, V3), HTM index = 13
     -  N2 = (V3, V0, V2), HTM index = 14
     -  N3 = (V2, V0, V1), HTM index = 15

    'S' denotes a triangle in the southern hemisphere,
    'N' denotes a triangle in the northern hemisphere.
 */


/* ---- Types ---- */

/*  HTM triangle vs. region classification codes.
 */
typedef enum {
    SCISQL_DISJOINT = 0,  /* HTM triangle disjoint from region */
    SCISQL_INTERSECT = 1, /* HTM triangle intersects region */
    SCISQL_CONTAINS = 2,  /* HTM triangle completely contains region */
    SCISQL_INSIDE = 3     /* HTM triangle completely inside region */
} _scisql_htmcov;

/*  A node (triangle/trixel) in an HTM tree.
 */
typedef struct {
  scisql_v3 mid_vert[3];    /* triangle edge mid-points */
  scisql_v3 mid_edge[3];    /* subdivision plane normals */
  const scisql_v3 *vert[3]; /* triangle vertex pointers */
  const scisql_v3 *edge[3]; /* triangle edge normal pointers */
  scisql_v3p *end;          /* temporary used for indexing */
  int64_t id;               /* HTM ID of the node */
  int child;                /* index of next child (0-3) */
} _scisql_htmnode;

/*  A root to leaf path in a depth-first traversal of an HTM tree.
 */
typedef struct {
  scisql_htmroot root;  /* ordinal of root triangle (0-7) */
  _scisql_htmnode node[SCISQL_HTM_MAX_LEVEL + 1];
} _scisql_htmpath;


/* ---- Data ---- */

/*  HTM root triangle vertices/edge plane normals.
 */
static const scisql_v3 _scisql_htm_v3[6] = {
  { 0.0,  0.0,  1.0 },
  { 1.0,  0.0,  0.0 },
  { 0.0,  1.0,  0.0 },
  {-1.0,  0.0,  0.0 },
  { 0.0, -1.0,  0.0 },
  { 0.0,  0.0, -1.0 }
};

#define SCISQL_HTM_Z  &_scisql_htm_v3[0]
#define SCISQL_HTM_X  &_scisql_htm_v3[1]
#define SCISQL_HTM_Y  &_scisql_htm_v3[2]
#define SCISQL_HTM_NX &_scisql_htm_v3[3]
#define SCISQL_HTM_NY &_scisql_htm_v3[4]
#define SCISQL_HTM_NZ &_scisql_htm_v3[5]

/*  Vertex pointers for the 3 vertices of each HTM root triangle.
 */
static const scisql_v3 * const _scisql_htm_root_vert[24] = {
  SCISQL_HTM_X,  SCISQL_HTM_NZ, SCISQL_HTM_Y,  /* S0 */
  SCISQL_HTM_Y,  SCISQL_HTM_NZ, SCISQL_HTM_NX, /* S1 */
  SCISQL_HTM_NX, SCISQL_HTM_NZ, SCISQL_HTM_NY, /* S2 */
  SCISQL_HTM_NY, SCISQL_HTM_NZ, SCISQL_HTM_X,  /* S3 */
  SCISQL_HTM_X,  SCISQL_HTM_Z,  SCISQL_HTM_NY, /* N0 */
  SCISQL_HTM_NY, SCISQL_HTM_Z,  SCISQL_HTM_NX, /* N1 */
  SCISQL_HTM_NX, SCISQL_HTM_Z,  SCISQL_HTM_Y,  /* N2 */
  SCISQL_HTM_Y,  SCISQL_HTM_Z,  SCISQL_HTM_X   /* N3 */
};

/*  Edge normal pointers for the 3 edge normals of each HTM root triangle.
 */
static const scisql_v3 * const _scisql_htm_root_edge[24] = {
  SCISQL_HTM_Y,  SCISQL_HTM_X,  SCISQL_HTM_NZ, /* S0 */
  SCISQL_HTM_NX, SCISQL_HTM_Y,  SCISQL_HTM_NZ, /* S1 */
  SCISQL_HTM_NY, SCISQL_HTM_NX, SCISQL_HTM_NZ, /* S2 */
  SCISQL_HTM_X,  SCISQL_HTM_NY, SCISQL_HTM_NZ, /* S3 */
  SCISQL_HTM_NY, SCISQL_HTM_X,  SCISQL_HTM_Z,  /* N0 */
  SCISQL_HTM_NX, SCISQL_HTM_NY, SCISQL_HTM_Z,  /* N1 */
  SCISQL_HTM_Y,  SCISQL_HTM_NX, SCISQL_HTM_Z,  /* N2 */
  SCISQL_HTM_X,  SCISQL_HTM_Y,  SCISQL_HTM_Z   /* N3 */
};


/* ---- Implementation details ---- */

/*  Sets path to the i-th HTM root triangle.
 */
SCISQL_INLINE void _scisql_htmpath_root(_scisql_htmpath *path,
                                        scisql_htmroot r)
{
    path->node[0].vert[0] = _scisql_htm_root_vert[r*3];
    path->node[0].vert[1] = _scisql_htm_root_vert[r*3 + 1];
    path->node[0].vert[2] = _scisql_htm_root_vert[r*3 + 2];
    path->node[0].edge[0] = _scisql_htm_root_edge[r*3];
    path->node[0].edge[1] = _scisql_htm_root_edge[r*3 + 1];
    path->node[0].edge[2] = _scisql_htm_root_edge[r*3 + 2];
    path->node[0].id = r + 8;
    path->node[0].child = 0;
    path->root = r;
}

/*  Computes the normalized average of two input vertices.
 */
SCISQL_INLINE void _scisql_htm_vertex(scisql_v3 *out,
                                      const scisql_v3 *v1,
                                      const scisql_v3 *v2)
{
    scisql_v3_add(out, v1, v2);
    scisql_v3_normalize(out, out);
}

/*  Computes quantities needed by _scisql_htmnode_make0(node).
 */
SCISQL_INLINE void _scisql_htmnode_prep0(_scisql_htmnode *node) {
    _scisql_htm_vertex(&node->mid_vert[1], node->vert[2], node->vert[0]);
    _scisql_htm_vertex(&node->mid_vert[2], node->vert[0], node->vert[1]);
    scisql_v3_rcross(&node->mid_edge[1], &node->mid_vert[2], &node->mid_vert[1]);
}

/*  Sets node[1] to child 0 of node[0]. Assumes _scisql_htmnode_prep0(node)
    has been called.
 */
SCISQL_INLINE void _scisql_htmnode_make0(_scisql_htmnode *node) {
    node[1].vert[0] = node[0].vert[0];
    node[1].vert[1] = &node[0].mid_vert[2];
    node[1].vert[2] = &node[0].mid_vert[1];
    node[1].edge[0] = node[0].edge[0];
    node[1].edge[1] = &node[0].mid_edge[1];
    node[1].edge[2] = node[0].edge[2];
    node[0].child = 1;
    node[1].id = node[0].id << 2;
    node[1].child = 0;
}

/*  Computes quantities needed by _scisql_htmnode_make1(node). Assumes
    _scisql_htmnode_prep0(node) has been called.
 */
SCISQL_INLINE void _scisql_htmnode_prep1(_scisql_htmnode *node) {
    _scisql_htm_vertex(&node->mid_vert[0], node->vert[1], node->vert[2]);
    scisql_v3_rcross(&node->mid_edge[2], &node->mid_vert[0], &node->mid_vert[2]);
}

/*  Sets node[1] to child 1 of node[0]. Assumes _scisql_htmnode_prep1(node)
    has been called.
 */
SCISQL_INLINE void _scisql_htmnode_make1(_scisql_htmnode *node) {
    node[1].vert[0] = node[0].vert[1];
    node[1].vert[1] = &node[0].mid_vert[0];
    node[1].vert[2] = &node[0].mid_vert[2];
    node[1].edge[0] = node[0].edge[1];
    node[1].edge[1] = &node[0].mid_edge[2];
    node[1].edge[2] = node[0].edge[0];
    node[0].child = 2;
    node[1].id = (node[0].id << 2) + 1;
    node[1].child = 0;
}

/*  Computes quantities needed by _scisql_htmnode_make2(node). Assumes
    _scisql_htmnode_prep1 has been called.
 */
SCISQL_INLINE void _scisql_htmnode_prep2(_scisql_htmnode *node) {
    scisql_v3_rcross(&node->mid_edge[0], &node->mid_vert[1], &node->mid_vert[0]);
}

/*  Sets node[1] to child 2 of node[0]. Assumes _scisql_htmnode_prep2(node)
    has been called.
 */
SCISQL_INLINE void _scisql_htmnode_make2(_scisql_htmnode *node) {
    node[1].vert[0] = node[0].vert[2];
    node[1].vert[1] = &node[0].mid_vert[1];
    node[1].vert[2] = &node[0].mid_vert[0];
    node[1].edge[0] = node[0].edge[2];
    node[1].edge[1] = &node[0].mid_edge[0];
    node[1].edge[2] = node[0].edge[1];
    node[0].child = 3;
    node[1].id = (node[0].id << 2) + 2;
    node[1].child = 0;
}

/*  Sets node[1] to child 3 of node[0]. Assumes _scisql_htmnode_prep2(node)
    has been called.
 */
SCISQL_INLINE void _scisql_htmnode_make3(_scisql_htmnode *node) {
    scisql_v3_neg(&node[0].mid_edge[0], &node[0].mid_edge[0]);
    scisql_v3_neg(&node[0].mid_edge[1], &node[0].mid_edge[1]);
    scisql_v3_neg(&node[0].mid_edge[2], &node[0].mid_edge[2]);
    node[1].vert[0] = &node[0].mid_vert[0];
    node[1].vert[1] = &node[0].mid_vert[1];
    node[1].vert[2] = &node[0].mid_vert[2];
    node[1].edge[0] = &node[0].mid_edge[0];
    node[1].edge[1] = &node[0].mid_edge[1];
    node[1].edge[2] = &node[0].mid_edge[2];
    node[0].child = 4;
    node[1].id = (node[0].id << 2) + 3;
    node[1].child = 0;
}

/*  Reorders the vectors in [begin,end) such that the resulting array can be
    partitioned into [begin,m) and [m,end), where all vectors in [begin,m) are
    inside the partitioning plane, and all vectors in [m, end) are outside.

    A pointer to the partitioning element m is returned.

    Assumes that plane != 0, begin != 0, end != 0, and begin <= end.
 */
static scisql_v3p * _scisql_htm_partition(const scisql_v3 *plane,
                                          scisql_v3p *beg,
                                          scisql_v3p *end)
{
    scisql_v3p tmp;
    for (; beg < end; ++beg) {
        if (scisql_v3_dot(plane, &beg->v) < 0.0) {
            /* beg is outside plane, find end which is inside,
               swap contents of beg and end. */
            for (--end; end > beg; --end) {
                if (scisql_v3_dot(plane, &end->v) >= 0.0) {
                    break;
                }
            }
            if (end <= beg) {
                break;
            }
            tmp = *beg;
            *beg = *end;
            *end = tmp;
        }
    }
    return beg;
}

/*  Perform a depth-first traversal of an HTM tree. At each step of the
    traversal, the input position list is partitioned into the list of
    points inside the current HTM node and those outside - the full
    depth-first traverals of the HTM tree therefore yields a list of
    positions sorted on their HTM index. This can be faster than computing
    HTM indexes one at a time when inputs are clustered spatially, since the
    boundary representation of a given HTM triangle is computed at most once.
 */
static void _scisql_htmpath_sort(_scisql_htmpath *path,
                                 scisql_v3p *begin,
                                 scisql_v3p *end,
                                 int64_t *ids,
                                 int level)
{
    _scisql_htmnode * const root = path->node;
    _scisql_htmnode * const leaf = root + level;
    _scisql_htmnode *curnode = path->node;
    scisql_v3p *beg = begin;

    curnode->end = end;

    while (1) {
        if (curnode != leaf) {
            /* Not a leaf node, so continue descending the tree.
               Mid-points and edge normals are computed on-demand. */
            int child = curnode->child;
            if (child == 0) {
                _scisql_htmnode_prep0(curnode);
                end = _scisql_htm_partition(&curnode->mid_edge[1], beg, end);
                if (beg < end) {
                    _scisql_htmnode_make0(curnode);
                    ++curnode;
                    curnode->end = end;
                    continue;
                }
                end = curnode->end;
            }
            if (child <= 1) {
                _scisql_htmnode_prep1(curnode);
                end = _scisql_htm_partition(&curnode->mid_edge[2], beg, end);
                if (beg < end) {
                    _scisql_htmnode_make1(curnode);
                    ++curnode;
                    curnode->end = end;
                    continue;
                }
                end = curnode->end;
            }
            if (child <= 2) {
                _scisql_htmnode_prep2(curnode);
                end = _scisql_htm_partition(&curnode->mid_edge[0], beg, end);
                if (beg < end) {
                    _scisql_htmnode_make2(curnode);
                    ++curnode;
                    curnode->end = end;
                    continue;
                }
                end = curnode->end;
            }
            if (beg < end) {
                _scisql_htmnode_make3(curnode);
                ++curnode;
                curnode->end = end;
                continue;
            }
        } else {
            /* reached a leaf triangle - all points between beg and end are
               inside and have the same HTM index. */
            int64_t id = curnode->id;
            size_t i = beg - begin;
            size_t j = end - begin;
            for (; i < j; ++i) {
                ids[i] = id;
            }
            beg = end;
        }
        /* walk back up the path until we find a node which
           still contains unsorted points. */
        do {
            if (curnode == root) {
                return;
            }
            --curnode;
            end = curnode->end;
        } while (beg == end);
    }
}

#define SCISQL_IDS_INIT_CAP 16

static scisql_ids * _scisql_ids_init() {
    scisql_ids *ids = (scisql_ids *) malloc(
        sizeof(scisql_ids) + 2 * SCISQL_IDS_INIT_CAP * sizeof(int64_t));
    if (ids != 0) {
        ids->n = 0;
        ids->cap = SCISQL_IDS_INIT_CAP;
    }
    return ids;
}

static scisql_ids * _scisql_ids_grow(scisql_ids *ids) {
    size_t cap = ids->cap;
    size_t nbytes = sizeof(scisql_ids) + 4 * cap * sizeof(int64_t);
    scisql_ids *out = (scisql_ids *) realloc(ids, nbytes);
    if (out != 0) {
        out->cap = 2 * cap;
    } else {
        free(ids);
    }
    return out;
}

SCISQL_INLINE scisql_ids * _scisql_ids_add(scisql_ids *ids,
                                           int64_t min_id,
                                           int64_t max_id)
{
    size_t n = ids->n;
    if (n == 0) {
        ids->n = 1;
        ids->ranges[0] = min_id;
        ids->ranges[1] = max_id;
    } else if (min_id == ids->ranges[2*n - 1] + 1) {
        ids->ranges[2*n - 1] = max_id;
    } else {
        if (n == ids->cap) {
            ids = _scisql_ids_grow(ids);
            if (ids == 0) {
                return 0;
            }
        }
        ids->n = n + 1;
        ids->ranges[2*n] = min_id;
        ids->ranges[2*n + 1] = max_id;
    }
    return ids;
}


/*  Returns the coverage code describing the spatial relationship between the
    given HTM triangle and spherical circle.
 */
static _scisql_htmcov _scisql_s2circle_htmcov(const _scisql_htmnode *node,
                                              const scisql_v3 *center,
                                              double dist2)
{
    int i0 = scisql_v3_edgedist2(center, node->vert[0], node->vert[1],
                                 node->edge[0]) <= dist2;
    int i1 = scisql_v3_edgedist2(center, node->vert[1], node->vert[2],
                                 node->edge[1]) <= dist2;
    int i2 = scisql_v3_edgedist2(center, node->vert[2], node->vert[0],
                                 node->edge[2]) <= dist2;
    if (i0 != i1 || i1 != i2) {
        return SCISQL_INTERSECT;
    }
    if (i0 == 1) {
        /* min distance to every edge is <= circle radius */
        return SCISQL_INSIDE;
    }
    /* min distance to every edge is > circle radius - circle
       is either inside triangle or disjoint from it */
    if (scisql_v3_dot(center, node->edge[0]) >= 0.0 &&
        scisql_v3_dot(center, node->edge[1]) >= 0.0 &&
        scisql_v3_dot(center, node->edge[2]) >= 0.0) {
        return SCISQL_CONTAINS;
    }
    return SCISQL_DISJOINT;
}

static const double SCISQL_INF = 1.0 / 0.0;
static const double SCISQL_NEG_INF = -1.0 / 0.0;

/*  Tests whether poly intersects the edge (v1, v2) with plane normal n.

    The idea is that a solution v = (x,y,z) must satisfy:

        v . n = 0, v != 0
        v . (n ^ v1) >= 0
        v . (v2 ^ n) >= 0
        v . e_i >= 0

    where e_i are the edge plane normals for the polygon, and (n ^ v1), (v2 ^ n) are
    plane normals that bound the lune defined by n, v1, and v2. Write this as:

        v . n = 0
        v . c_i >= 0

    Now assume nz > 0 (at least one of nx, ny, nz must be non-zero, and negative values
    are dealt with in similar fashion). Use the equality to obtain

        z = - (x * nx + y * ny) / nz

    then substitute into the inequalities to obtain:

        x * (c_ix * nz - c_iz * nx) + y * (c_iy * nz - c_iz * ny) >= 0

    which we write

        x * a_i + y * b_i >= 0

    If a solution v exists, then Kv is also a solution (for positive scalar K),
    so we can fix y = 1 and look for solutions to

        x * a_i + b_i >= 0

    If there are none, fix y = -1 and look for solutions to:

        x * a_i - b_i >= 0

    If again there are none, then y = 0, and the problem reduces to checking whether

        x * a_i >= 0

    has any solutions (this is the case when the non-zero a_i have the same sign).
 */
static int _scisql_isect_test(const scisql_v3 *v1,
                              const scisql_v3 *v2,
                              const scisql_v3 *n,
                              const scisql_s2cpoly *poly)
{
    double ab[2*(SCISQL_MAX_VERTS + 2)];
    scisql_v3 c0;
    scisql_v3 c1;
    double min_1, max_1, min_m1, max_m1;
    size_t i, neg, pos;

    scisql_v3_cross(&c0, n, v1);
    scisql_v3_cross(&c1, v2, n);
    if (n->z != 0.0) {
        double s = (n->z > 0.0) ? 1.0 : -1.0;
        ab[0] = s * (c0.x * n->z - c0.z * n->x);
        ab[1] = s * (c0.y * n->z - c0.z * n->y);
        ab[2] = s * (c1.x * n->z - c1.z * n->x);
        ab[3] = s * (c1.y * n->z - c1.z * n->y);
        for (i = 0; i < poly->n; ++i) {
            ab[2*i + 4] = s * (poly->edges[i].x * n->z - poly->edges[i].z * n->x);
            ab[2*i + 5] = s * (poly->edges[i].y * n->z - poly->edges[i].z * n->y);
        }
    } else if (n->y != 0.0) {
        double s = (n->y > 0.0) ? 1.0 : -1.0;
        ab[0] = s * (c0.x * n->y - c0.y * n->x);
        ab[1] = s * (c0.z * n->y);
        ab[2] = s * (c1.x * n->y - c1.y * n->x);
        ab[3] = s * (c1.z * n->y);
        for (i = 0; i < poly->n; ++i) {
            ab[2*i + 4] = s * (poly->edges[i].x * n->y - poly->edges[i].y * n->x);
            ab[2*i + 5] = s * (poly->edges[i].z * n->y);
        }
    } else if (n->x != 0.0) {
        double s = (n->x > 0.0) ? 1.0 : -1.0;
        ab[0] = s * (c0.y * n->x);
        ab[1] = s * (c0.z * n->x);
        ab[2] = s * (c1.y * n->x);
        ab[3] = s * (c1.z * n->x);
        for (i = 0; i < poly->n; ++i) {
            ab[2*i + 4] = s * (poly->edges[i].y * n->x);
            ab[2*i + 5] = s * (poly->edges[i].z * n->x);
        }
    } else {
        return 0;
    }
    /* search for solutions to a*x +/- b >= 0, with constraint coeffs stored in
       ab */
    min_1 = min_m1 = SCISQL_NEG_INF;
    max_1 = max_m1 = SCISQL_INF;
    for (i = 0, neg = 0, pos = 0; i < poly->n + 2; ++i) {
        double a = ab[2*i];
        double b = ab[2*i + 1];
        if (a == 0.0) {
            if (b < 0.0) {
                min_1 = SCISQL_INF;
                max_1 = SCISQL_NEG_INF;
            } else if (b > 0.0) {
                min_m1 = SCISQL_INF;
                max_m1 = SCISQL_NEG_INF;
            }
        } else if (a < 0.0) {
            ++neg;
            double d = -b / a;
            if (d < max_1) {
                max_1 = d;
            }
            if (-d < max_m1) {
                max_m1 = -d;
            }
        } else {
            ++pos;
            double d = -b / a;
            if (d > min_1) {
                min_1 = d;
            }
            if (-d > min_m1) {
                min_m1 = -d;
            }
        }
    }
    if (min_1 <= max_1 || min_m1 <= max_m1) {
        return 1;
    }
    return (neg == 0 || pos == 0);
}

/*  Returns the coverage code describing the spatial relationship between the
    given HTM triangle and spherical convex polygon.
 */
static _scisql_htmcov _scisql_s2cpoly_htmcov(const _scisql_htmnode *node,
                                             const scisql_s2cpoly *poly)
{
    int i0 = scisql_s2cpoly_cv3(poly, node->vert[0]);
    int i1 = scisql_s2cpoly_cv3(poly, node->vert[1]);
    int i2 = scisql_s2cpoly_cv3(poly, node->vert[2]);

    if (i0 != i1 || i1 != i2) {
        return SCISQL_INTERSECT;
    }
    /* If all triangle vertices are inside poly, then triangle is inside
       by convexity. */
    if (i0 == 1) {
        return SCISQL_INSIDE;
    }
    if (_scisql_isect_test(node->vert[0], node->vert[1], node->edge[0], poly) != 0 ||
        _scisql_isect_test(node->vert[1], node->vert[2], node->edge[1], poly) != 0 ||
        _scisql_isect_test(node->vert[2], node->vert[0], node->edge[2], poly) != 0) {
        return SCISQL_INTERSECT;
    }
    /* All triangle vertices are outside poly and there are no edge/edge
       intersections. Polygon is either inside triangle or disjoint from
       it */
    if (scisql_v3_dot(&poly->vsum, node->edge[0]) >= 0.0 &&
        scisql_v3_dot(&poly->vsum, node->edge[1]) >= 0.0 &&
        scisql_v3_dot(&poly->vsum, node->edge[2]) >= 0.0) {
        return SCISQL_CONTAINS;
    }
    return SCISQL_DISJOINT;
}

/*  Returns the HTM root triangle for a point.
 */
SCISQL_INLINE scisql_htmroot _scisql_v3_htmroot(const scisql_v3 *v) {
    if (v->z < 0.0) {
        /* S0, S1, S2, S3 */
        if (v->y > 0.0) {
            return (v->x > 0.0) ? SCISQL_HTM_S0 : SCISQL_HTM_S1;
        } else if (v->y == 0.0) {
            return (v->x >= 0.0) ? SCISQL_HTM_S0 : SCISQL_HTM_S2;
        } else {
            return (v->x < 0.0) ? SCISQL_HTM_S2 : SCISQL_HTM_S3;
        }
    } else {
        /* N0, N1, N2, N3 */
        if (v->y > 0.0) {
            return (v->x > 0.0) ? SCISQL_HTM_N3 : SCISQL_HTM_N2;
        } else if (v->y == 0.0) {
            return (v->x >= 0.0) ? SCISQL_HTM_N3 : SCISQL_HTM_N1;
        } else {
            return (v->x < 0.0) ? SCISQL_HTM_N1 : SCISQL_HTM_N0;
        }
    }
}

/*  Partitions an array of points according to their root triangle numbers.
 */
static size_t _scisql_htm_rootpart(scisql_v3p *points,
                                   unsigned char *ids,
                                   size_t n,
                                   scisql_htmroot root)
{
    scisql_v3p tmp;
    size_t beg, end;
    unsigned char c;
    for (beg = 0, end = n; beg < end; ++beg) {
        if (ids[beg] >= root) {
            for (; end > beg; --end) {
                if (ids[end - 1] < root) {
                    break;
                }
            }
            if (end == beg) {
                break;
            }
            tmp = points[beg];
            points[beg] = points[end - 1];
            points[end - 1] = tmp;
            c = ids[beg];
            ids[beg] = ids[end - 1];
            ids[end - 1] = c;
        }
    }
    return beg;
}

/*  Sorts the given array of positions by root triangle number.
 */
static void _scisql_htm_rootsort(size_t roots[SCISQL_HTM_NROOTS + 1],
                                 scisql_v3p *points,
                                 unsigned char *ids,
                                 size_t n)
{
    size_t i, n0, n2, s2;

    /* compute root ids for all points */
    for (i = 0; i < n; ++i) {
        ids[i] = (unsigned char) _scisql_v3_htmroot(&points[i].v);
    }
    n0 = _scisql_htm_rootpart(points, ids, n, SCISQL_HTM_N0);
    s2 = _scisql_htm_rootpart(points, ids, n0, SCISQL_HTM_S2);
    roots[SCISQL_HTM_S0] = 0;
    roots[SCISQL_HTM_S1] = _scisql_htm_rootpart(points, ids, s2, SCISQL_HTM_S1);
    roots[SCISQL_HTM_S2] = s2;
    roots[SCISQL_HTM_S3] = _scisql_htm_rootpart(points + s2, ids + s2, n0 - s2,
                                                SCISQL_HTM_S3) + s2;
    n2 = _scisql_htm_rootpart(points + n0, ids + n0, n - n0, SCISQL_HTM_N2) + n0;
    roots[SCISQL_HTM_N0] = n0;
    roots[SCISQL_HTM_N1] = _scisql_htm_rootpart(points + n0, ids + n0, n2 - n0,
                                                SCISQL_HTM_N1) + n0;
    roots[SCISQL_HTM_N2] = n2;
    roots[SCISQL_HTM_N3] = _scisql_htm_rootpart(points + n2, ids + n2, n - n2,
                                                SCISQL_HTM_N3) + n2;
    roots[SCISQL_HTM_NROOTS] = n;
}

/*  Reduces the effective subdivision level of an HTM id range list by n levels
    and merges adjacent ranges. This typically reduces the number of ranges in
    the list, but also makes it a poorer approximation of the underlying
    geometry. Note that with sufficiently large n, any range list can be shrunk
    down to at most 4 ranges.

    In detail: a range [I1, I2] is mapped to [I1 & ~mask, I2 | mask], where
    mask = (1 << 2*n) - 1.
 */
static void _scisql_simplify_ids(scisql_ids *ids, int n) {
    size_t i, j, nr;
    int64_t mask;
    if (n <= 0 || ids == 0 || ids->n == 0) {
        return;
    }
    mask = (((int64_t) 1) << 2*n) - 1;
    for (i = 0, j = 0, nr = ids->n; i < nr; ++i, ++j) {
        int64_t idmin = ids->ranges[2*i] & ~mask;
        int64_t idmax = ids->ranges[2*i + 1] | mask;
        for (; i < nr - 1; ++i) {
            int64_t next = ids->ranges[2*i + 2] & ~mask;
            if (next > idmax + 1) {
                break;
            }
            idmax = ids->ranges[2*i + 3] | mask;
        }
        ids->ranges[2*j] = idmin;
        ids->ranges[2*j + 1] = idmax;
    }
    ids->n = j;
}


/* ---- API ---- */

SCISQL_LOCAL int64_t scisql_v3_htmid(const scisql_v3 *point, int level) {
    scisql_v3 v0, v1, v2;
    scisql_v3 sv0, sv1, sv2;
    scisql_v3 e;
    int64_t id;
    int curlevel;
    scisql_htmroot r;

    if (point == 0 || level < 0 || level > SCISQL_HTM_MAX_LEVEL) {
        return -1;
    }
    r = _scisql_v3_htmroot(point);
    v0 = *_scisql_htm_root_vert[r*3];
    v1 = *_scisql_htm_root_vert[r*3 + 1];
    v2 = *_scisql_htm_root_vert[r*3 + 2];
    id = r + 8;
    for (curlevel = 0; curlevel < level; ++curlevel) {
        _scisql_htm_vertex(&sv1, &v2, &v0);
        _scisql_htm_vertex(&sv2, &v0, &v1);
        scisql_v3_rcross(&e, &sv2, &sv1);
        if (scisql_v3_dot(&e, point) >= 0) {
            v1 = sv2;
            v2 = sv1;
            id = id << 2;
            continue;
        }
        _scisql_htm_vertex(&sv0, &v1, &v2);
        scisql_v3_rcross(&e, &sv0, &sv2);
        if (scisql_v3_dot(&e, point) >= 0) {
            v0 = v1;
            v1 = sv0;
            v2 = sv2;
            id = (id << 2) + 1;
            continue;
        }
        scisql_v3_rcross(&e, &sv1, &sv0);
        if (scisql_v3_dot(&e, point) >= 0) {
            v0 = v2;
            v1 = sv1;
            v2 = sv0;
            id = (id << 2) + 2;
        } else {
            v0 = sv0;
            v1 = sv1;
            v2 = sv2;
            id = (id << 2) + 3;
        }
    }
    return id;
}


SCISQL_LOCAL int scisql_v3p_htmsort(scisql_v3p *points,
                                    int64_t *ids,
                                    size_t n,
                                    int level)
{
    _scisql_htmpath path;
    size_t roots[SCISQL_HTM_NROOTS + 1];
    scisql_htmroot r;

    if (n == 0) {
        return 0;
    }
    if (points == 0 || ids == 0 || level < 0 || level > SCISQL_HTM_MAX_LEVEL) {
        return 1;
    }
    _scisql_htm_rootsort(roots, points, (unsigned char *) ids, n);
    for (r = SCISQL_HTM_S0; r <= SCISQL_HTM_N3; ++r) {
        if (roots[r] < roots[r + 1]) {
            _scisql_htmpath_root(&path, r);
            _scisql_htmpath_sort(&path, points + roots[r],
                                 points + roots[r + 1], ids + roots[r], level);
        }
    }
    return 0;
}


SCISQL_LOCAL scisql_ids * scisql_s2circle_htmids(scisql_ids *ids,
                                                 const scisql_v3 *center,
                                                 double radius,
                                                 int level,
                                                 size_t maxranges)
{
    _scisql_htmpath path;
    double dist2;
    scisql_htmroot root;
    int efflevel;

    if (center == 0 || level < 0 || level > SCISQL_HTM_MAX_LEVEL) {
        return 0;
    }
    if (ids == 0) {
        ids = _scisql_ids_init();
        if (ids == 0) {
            return 0;
        }
    } else {
        ids->n = 0;
    }
    /* Deal with degenerate cases */
    if (radius < 0.0) {
        /* empty ID list */
        return ids;
    } else if (radius >= 180.0) {
        /* the entire sky */
        int64_t min_id = (8 + SCISQL_HTM_S0) << level * 2;
        int64_t max_id = ((8 + SCISQL_HTM_NROOTS) << level * 2) - 1;
        return _scisql_ids_add(ids, min_id, max_id);
    }

    efflevel = level;
    /* compute square of secant distance corresponding to radius */
    dist2 = sin(radius * 0.5 * SCISQL_RAD_PER_DEG);
    dist2 = 4.0 * dist2 * dist2;

    for (root = SCISQL_HTM_S0; root <= SCISQL_HTM_N3; ++root) {
        _scisql_htmnode *curnode = path.node;
        int curlevel = 0;

        _scisql_htmpath_root(&path, root);

        while (1) {
            _scisql_htmcov cov = _scisql_s2circle_htmcov(curnode, center, dist2);
            switch (cov) {
                case SCISQL_CONTAINS:
                    if (curlevel == 0) {
                        /* no need to consider other roots */
                        root = SCISQL_HTM_N3;
                    } else {
                        /* no need to consider other children of parent */
                        curnode[-1].child = 4;
                    }
                    /* fall-through */
                case SCISQL_INTERSECT:
                    if (curlevel < efflevel) {
                        /* continue subdividing */
                        _scisql_htmnode_prep0(curnode);
                        _scisql_htmnode_make0(curnode);
                        ++curnode;
                        ++curlevel;
                        continue;
                    }
                    /* fall-through */
                case SCISQL_INSIDE:
                    /* reached a leaf or fully covered HTM triangle,
                       append HTM ID range to results */
                    {
                        int64_t id = curnode->id << (level - curlevel) * 2;
                        int64_t n = ((int64_t) 1) << (level - curlevel) * 2;
                        ids = _scisql_ids_add(ids, id, id + n - 1);
                    }
                    if (ids == 0) {
                        return ids;
                    }
                    while (ids->n > maxranges && efflevel != 0) {
                        /* too many ranges: reduce effective subdivision level */
                        --efflevel;
                        if (curlevel > efflevel) {
                           curnode = curnode - (curlevel - efflevel);
                           curlevel = efflevel; 
                        }
                        _scisql_simplify_ids(ids, level - efflevel);
                    }
                    break;
                default:
                    /* HTM triangle does not intersect circle */
                    break;
            }
            /* ascend towards the root */
            --curlevel;
            --curnode;
            while (curlevel >= 0 && curnode->child == 4) {
                --curnode;
                --curlevel;
            }
            if (curlevel < 0) {
                /* finished with this root */
                break;
            }
            if (curnode->child == 1) {
                _scisql_htmnode_prep1(curnode);
                _scisql_htmnode_make1(curnode);
            } else if (curnode->child == 2) {
                _scisql_htmnode_prep2(curnode);
                _scisql_htmnode_make2(curnode);
            } else {
                _scisql_htmnode_make3(curnode);
            }
            ++curnode;
            ++curlevel;
        }
    }
    return ids;
}


SCISQL_LOCAL scisql_ids * scisql_s2cpoly_htmids(scisql_ids * ids,
                                                const scisql_s2cpoly *poly,
                                                int level,
                                                size_t maxranges)
{
    _scisql_htmpath path;
    scisql_htmroot root;
    int efflevel;

    if (poly == 0 || level < 0 || level > SCISQL_HTM_MAX_LEVEL) {
        return 0;
    }
    if (ids == 0) {
        ids = _scisql_ids_init();
        if (ids == 0) {
            return 0;
        }
    } else {
        ids->n = 0;
    }

    efflevel = level;

    for (root = SCISQL_HTM_S0; root <= SCISQL_HTM_N3; ++root) {
        _scisql_htmnode *curnode = path.node;
        int curlevel = 0;

        _scisql_htmpath_root(&path, root);

        while (1) {
            _scisql_htmcov cov = _scisql_s2cpoly_htmcov(curnode, poly);
            switch (cov) {
                case SCISQL_CONTAINS:
                    if (curlevel == 0) {
                        /* no need to consider other roots */
                        root = SCISQL_HTM_N3;
                    } else {
                        /* no need to consider other children of parent */
                        curnode[-1].child = 4;
                    }
                    /* fall-through */
                case SCISQL_INTERSECT:
                    if (curlevel < efflevel) {
                        /* continue subdividing */
                        _scisql_htmnode_prep0(curnode);
                        _scisql_htmnode_make0(curnode);
                        ++curnode;
                        ++curlevel;
                        continue;
                    }
                    /* fall-through */
                case SCISQL_INSIDE:
                    /* reached a leaf or fully covered HTM triangle,
                       append HTM ID range to results */
                    {
                        int64_t id = curnode->id << (level - curlevel) * 2;
                        int64_t n = ((int64_t) 1) << (level - curlevel) * 2;
                        ids = _scisql_ids_add(ids, id, id + n - 1);
                    }
                    if (ids == 0) {
                        return ids;
                    }
                    while (ids->n > maxranges && efflevel != 0) {
                        /* too many ranges: reduce effetive subdivision level */
                        --efflevel;
                        if (curlevel > efflevel) {
                           curnode = curnode - (curlevel - efflevel);
                           curlevel = efflevel;
                        }
                        _scisql_simplify_ids(ids, level - efflevel);
                    }
                    break;
                default:
                    /* HTM triangle does not intersect polygon */
                    break;
            }
            /* ascend towards the root */
            --curlevel;
            --curnode;
            while (curlevel >= 0 && curnode->child == 4) {
                --curnode;
                --curlevel;
            }
            if (curlevel < 0) {
                /* finished with this root */
                break;
            }
            if (curnode->child == 1) {
                _scisql_htmnode_prep1(curnode);
                _scisql_htmnode_make1(curnode);
            } else if (curnode->child == 2) {
                _scisql_htmnode_prep2(curnode);
                _scisql_htmnode_make2(curnode);
            } else {
                _scisql_htmnode_make3(curnode);
            }
            ++curnode;
            ++curlevel;
        }
    }
    return ids;
}


#ifdef __cplusplus
}
#endif


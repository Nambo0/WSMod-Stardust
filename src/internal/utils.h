#pragma once

#include "mkb.h"
#define LEN(array) (sizeof(array) / sizeof((array)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) (MIN((max), MAX((min), (x))))

/*
 * Vector macros which may have existed in the original source in some form.
 */
#define VEC_ADD(v1, v2)                                                        \
  (Vec{(v1).x + (v2).x, (v1).y + (v2).y, (v1).z + (v2).z})
#define VEC_SUB(v1, v2)                                                        \
  (Vec{(v1).x - (v2).x, (v1).y - (v2).y, (v1).z - (v2).z})
#define VEC_SUB2D(v1, v2) (Vec{(v1).x - (v2).x, 0, (v1).z - (v2).z})
#define VEC_SCALE(scale, v)                                                    \
  (Vec{(scale) * (v).x, (scale) * (v).y, (scale) * (v).z})
#define VEC_DOT(v1, v2) ((v1).x * (v2).x + (v1).y * (v2).y + (v1).z * (v2).z)
#define VEC_LEN_SQ(v) (VEC_DOT((v), (v)))
#define VEC_ZERO (Vec{0, 0, 0})
#define VEC_DIST_SQ(v1, v2) (VEC_LEN_SQ(VEC_SUB((v1), (v2))))
#ifndef VEC3_H
#define VEC3_H

#include <math.h>

typedef struct Vec3 {
    float x;
    float y;
    float z;
} Vec3;

static inline Vec3 v3(float x, float y, float z)
{
    Vec3 v = {x, y, z};
    return v;
}

static inline Vec3 v3_add(Vec3 a, Vec3 b)
{
    return v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline Vec3 v3_sub(Vec3 a, Vec3 b)
{
    return v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline Vec3 v3_mul(Vec3 a, float s)
{
    return v3(a.x * s, a.y * s, a.z * s);
}

static inline float v3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float v3_len(Vec3 a)
{
    return sqrtf(v3_dot(a, a));
}

static inline Vec3 v3_cross(Vec3 a, Vec3 b)
{
    return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x);
}

#endif

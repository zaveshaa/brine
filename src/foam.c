#include "brine_internal.h"

#include <math.h>
#include <stdlib.h>

static const float V_SPRAY = 0.9f;
static const float DEF_SURF = 0.35f;
static const float RATE_SPRAY = 0.05f;
static const float RATE_FOAM = 0.004f;
static const float LIFE_SPRAY = 1.2f;
static const float LIFE_FOAM = 3.0f;
static const float DRAG_SPRAY = 0.5f;
static const float DRAG_FOAM = 3.0f;
static const float COUPLE_SPRAY = 1.0f;
static const float COUPLE_FOAM = 8.0f;
static const float BUOY_FOAM = 1.5f;

static unsigned int rnd(Brine *b)
{
    unsigned int x = b->rng;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    b->rng = x;
    return x;
}

static float frand(Brine *b)
{
    return (float)(rnd(b) >> 8) * (1.0f / 16777216.0f);
}

int foam_reserve(Brine *b, size_t n)
{
    size_t want;
    void *p;

    if (n <= b->scap) {
        return 0;
    }
    want = (b->scap != 0) ? b->scap : 64;
    while (want < n) {
        want *= 2;
    }
    p = realloc(b->spos, want * sizeof *b->spos);
    if (p == NULL) {
        return -1;
    }
    b->spos = p;
    p = realloc(b->svel, want * sizeof *b->svel);
    if (p == NULL) {
        return -1;
    }
    b->svel = p;
    p = realloc(b->sage, want * sizeof *b->sage);
    if (p == NULL) {
        return -1;
    }
    b->sage = p;
    p = realloc(b->stype, want * sizeof *b->stype);
    if (p == NULL) {
        return -1;
    }
    b->stype = p;
    b->scap = want;
    return 0;
}

static Vec3 sample_vel(const Brine *b, Vec3 p, float *wsum_out)
{
    int gx = b->gx;
    int gy = b->gy;
    int gz = b->gz;
    float h2 = b->h2;
    float coef6 = b->coef6;
    Vec3 acc = v3(0.0f, 0.0f, 0.0f);
    float wsum = 0.0f;
    int c = grid_cell(b, p);
    int ix = c % gx;
    int iy = (c / gx) % gy;
    int iz = c / (gx * gy);
    int dz;

    for (dz = -1; dz <= 1; dz++) {
        int z = iz + dz;
        int dy;
        if (z < 0 || z >= gz) {
            continue;
        }
        for (dy = -1; dy <= 1; dy++) {
            int y = iy + dy;
            int dx;
            if (y < 0 || y >= gy) {
                continue;
            }
            for (dx = -1; dx <= 1; dx++) {
                int x = ix + dx;
                int cell;
                int k;
                if (x < 0 || x >= gx) {
                    continue;
                }
                cell = (z * gy + y) * gx + x;
                for (k = b->cell_start[cell]; k < b->cell_start[cell + 1]; k++) {
                    int j = b->order[k];
                    Vec3 r = v3_sub(p, b->pos[j]);
                    float r2 = v3_dot(r, r);
                    if (r2 < h2) {
                        float d = h2 - r2;
                        float w = coef6 * d * d * d;
                        acc = v3_add(acc, v3_mul(b->vel[j], w));
                        wsum += w;
                    }
                }
            }
        }
    }
    *wsum_out = wsum;
    return acc;
}

void foam_emit(Brine *b, float dt)
{
    size_t n = b->count;
    size_t budget = b->count;
    float ref = (float)b->neigh_ref;
    int gx = b->gx;
    int gy = b->gy;
    int gz = b->gz;
    float h2 = b->h2;
    size_t i;

    if (budget <= b->scount) {
        return;
    }
    (void)dt;
    for (i = 0; i < n; i++) {
        Vec3 pi = b->pos[i];
        float deficit;
        float v;
        float p;
        unsigned char type;
        int neigh = 0;
        int c = b->cell_of[i];
        int ix = c % gx;
        int iy = (c / gx) % gy;
        int iz = c / (gx * gy);
        int dz;

        if (b->scount >= budget) {
            break;
        }
        for (dz = -1; dz <= 1; dz++) {
            int z = iz + dz;
            int dy;
            if (z < 0 || z >= gz) {
                continue;
            }
            for (dy = -1; dy <= 1; dy++) {
                int y = iy + dy;
                int dx;
                if (y < 0 || y >= gy) {
                    continue;
                }
                for (dx = -1; dx <= 1; dx++) {
                    int x = ix + dx;
                    int cell;
                    int k;
                    if (x < 0 || x >= gx) {
                        continue;
                    }
                    cell = (z * gy + y) * gx + x;
                    for (k = b->cell_start[cell]; k < b->cell_start[cell + 1]; k++) {
                        int j = b->order[k];
                        Vec3 r;
                        if (j == (int)i) {
                            continue;
                        }
                        r = v3_sub(pi, b->pos[j]);
                        if (v3_dot(r, r) < h2) {
                            neigh++;
                        }
                    }
                }
            }
        }
        deficit = 1.0f - (float)neigh / ref;
        if (deficit <= DEF_SURF) {
            continue;
        }
        v = v3_len(b->vel[i]);
        if (v > V_SPRAY) {
            type = (unsigned char)BRINE_SPRAY;
            p = RATE_SPRAY * deficit * (v / V_SPRAY);
        } else {
            type = (unsigned char)BRINE_FOAM;
            p = RATE_FOAM * deficit;
        }
        if (p > 0.5f) {
            p = 0.5f;
        }
        if (frand(b) < p) {
            if (foam_reserve(b, b->scount + 1) != 0) {
                return;
            }
            b->spos[b->scount] = b->pos[i];
            b->svel[b->scount] = b->vel[i];
            b->sage[b->scount] = 0.0f;
            b->stype[b->scount] = type;
            b->scount++;
        }
    }
}

void foam_advance(Brine *b, float dt)
{
    float g = b->s.gravity;
    Vec3 lo = b->s.bmin;
    Vec3 hi = b->s.bmax;
    size_t i = 0;

    while (i < b->scount) {
        unsigned char type = b->stype[i];
        Vec3 p = b->spos[i];
        Vec3 v = b->svel[i];
        float age = b->sage[i] + dt;
        float wsum;
        Vec3 vf = sample_vel(b, p, &wsum);

        if (wsum > 0.0f) {
            vf = v3_mul(vf, 1.0f / wsum);
        }
        v.y -= g * dt;
        if (type == (unsigned char)BRINE_SPRAY) {
            if (wsum > 0.0f) {
                float k = COUPLE_SPRAY * dt;
                if (k > 1.0f) {
                    k = 1.0f;
                }
                v = v3_add(v, v3_mul(v3_sub(vf, v), k));
            }
            v = v3_mul(v, 1.0f - DRAG_SPRAY * dt);
            if (v3_len(v) < 0.4f && age > 0.3f) {
                type = (unsigned char)BRINE_FOAM;
                age = 0.0f;
            }
        } else {
            if (wsum > 0.0f) {
                float k = COUPLE_FOAM * dt;
                if (k > 1.0f) {
                    k = 1.0f;
                }
                v = v3_add(v, v3_mul(v3_sub(vf, v), k));
            }
            v = v3_mul(v, 1.0f - DRAG_FOAM * dt);
            v.y += BUOY_FOAM * dt;
        }
        p = v3_add(p, v3_mul(v, dt));

        if (p.y < lo.y) {
            p.y = lo.y;
            v.y = 0.0f;
            if (type == (unsigned char)BRINE_SPRAY) {
                type = (unsigned char)BRINE_FOAM;
                age = 0.0f;
            }
        }
        if (p.x < lo.x || p.x > hi.x || p.y > hi.y || p.z < lo.z || p.z > hi.z) {
            b->scount--;
            b->spos[i] = b->spos[b->scount];
            b->svel[i] = b->svel[b->scount];
            b->sage[i] = b->sage[b->scount];
            b->stype[i] = b->stype[b->scount];
            continue;
        }
        if (age > (type == (unsigned char)BRINE_SPRAY ? LIFE_SPRAY : LIFE_FOAM)) {
            b->scount--;
            b->spos[i] = b->spos[b->scount];
            b->svel[i] = b->svel[b->scount];
            b->sage[i] = b->sage[b->scount];
            b->stype[i] = b->stype[b->scount];
            continue;
        }
        b->spos[i] = p;
        b->svel[i] = v;
        b->sage[i] = age;
        b->stype[i] = type;
        i++;
    }
}

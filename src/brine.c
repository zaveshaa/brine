#include "brine_internal.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

BrineParams brine_default_params(void)
{
    BrineParams p;
    int i;

    memset(&p, 0, sizeof p);
    p.gravity = 9.81f;
    p.rest_density = 1000.0f;
    p.h = 0.16f;
    p.mass = 0.0f;
    p.epsilon = 100.0f;
    p.viscosity = 0.03f;
    p.restitution = 0.05f;
    p.damping = 0.01f;
    p.surface_tension = 0.1f;
    p.vorticity = 0.1f;
    p.iterations = 4;
    p.secondary = 0;
    for (i = 0; i < 3; i++) {
        p.bounds_min[i] = -1.0f;
        p.bounds_max[i] = 1.0f;
    }
    p.bounds_min[1] = 0.0f;
    p.bounds_max[1] = 2.0f;
    return p;
}

Brine *brine_new(const BrineParams *p)
{
    BrineParams d;
    Brine *b;

    d = (p != NULL) ? *p : brine_default_params();
    b = calloc(1, sizeof *b);
    if (b == NULL) {
        return NULL;
    }
    b->s.gravity = d.gravity;
    b->s.rest_density = d.rest_density;
    b->s.h = d.h;
    b->s.mass = d.mass;
    b->s.epsilon = d.epsilon;
    b->s.viscosity = d.viscosity;
    b->s.restitution = d.restitution;
    b->s.damping = d.damping;
    b->s.surface_tension = d.surface_tension;
    b->s.vorticity = d.vorticity;
    b->s.spacing = 0.0f;
    b->s.iterations = (d.iterations > 0) ? d.iterations : 1;
    b->s.secondary = (d.secondary != 0) ? 1 : 0;
    b->rng = 0x9e3779b9u;
    b->s.bmin = v3(d.bounds_min[0], d.bounds_min[1], d.bounds_min[2]);
    b->s.bmax = v3(d.bounds_max[0], d.bounds_max[1], d.bounds_max[2]);
    kernel_constants(b);
    return b;
}

void brine_free(Brine *b)
{
    if (b == NULL) {
        return;
    }
    free(b->pos);
    free(b->vel);
    free(b->pred);
    free(b->delta);
    free(b->omega);
    free(b->rho);
    free(b->lambda);
    free(b->omega_mag);
    free(b->cell_count);
    free(b->cell_start);
    free(b->cursor);
    free(b->cell_of);
    free(b->order);
    free(b->spos);
    free(b->svel);
    free(b->sage);
    free(b->stype);
    free(b);
}

void brine_clear(Brine *b)
{
    b->count = 0;
    b->scount = 0;
}

int brine_reserve(Brine *b, size_t n)
{
    void *p;

    if (n <= b->cap) {
        return 0;
    }
    p = realloc(b->pos, n * sizeof *b->pos);
    if (p == NULL) {
        return -1;
    }
    b->pos = p;
    p = realloc(b->vel, n * sizeof *b->vel);
    if (p == NULL) {
        return -1;
    }
    b->vel = p;
    p = realloc(b->pred, n * sizeof *b->pred);
    if (p == NULL) {
        return -1;
    }
    b->pred = p;
    p = realloc(b->delta, n * sizeof *b->delta);
    if (p == NULL) {
        return -1;
    }
    b->delta = p;
    p = realloc(b->omega, n * sizeof *b->omega);
    if (p == NULL) {
        return -1;
    }
    b->omega = p;
    p = realloc(b->rho, n * sizeof *b->rho);
    if (p == NULL) {
        return -1;
    }
    b->rho = p;
    p = realloc(b->lambda, n * sizeof *b->lambda);
    if (p == NULL) {
        return -1;
    }
    b->lambda = p;
    p = realloc(b->omega_mag, n * sizeof *b->omega_mag);
    if (p == NULL) {
        return -1;
    }
    b->omega_mag = p;
    b->cap = n;
    return 0;
}

static void set_spacing(Brine *b, float spacing)
{
    int R = (int)(b->s.h / spacing) + 2;
    float h2 = b->s.h * b->s.h;
    int ref = 0;
    int dx;
    int dy;
    int dz;

    if (b->s.mass <= 0.0f) {
        b->s.mass = b->s.rest_density * spacing * spacing * spacing;
    }
    b->s.spacing = spacing;
    for (dz = -R; dz <= R; dz++) {
        for (dy = -R; dy <= R; dy++) {
            for (dx = -R; dx <= R; dx++) {
                float d2;
                if (dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }
                d2 = (float)(dx * dx + dy * dy + dz * dz) * spacing * spacing;
                if (d2 < h2) {
                    ref++;
                }
            }
        }
    }
    b->neigh_ref = (ref > 0) ? ref : 1;
}

static void push_particle(Brine *b, Vec3 q)
{
    size_t i = b->count;

    b->pos[i] = q;
    b->vel[i] = v3(0.0f, 0.0f, 0.0f);
    b->pred[i] = q;
    b->delta[i] = v3(0.0f, 0.0f, 0.0f);
    b->omega[i] = v3(0.0f, 0.0f, 0.0f);
    b->rho[i] = b->s.rest_density;
    b->lambda[i] = 0.0f;
    b->omega_mag[i] = 0.0f;
    b->count++;
}

int brine_emit_box(Brine *b, const float lo[3], const float hi[3], float spacing)
{
    float ext[3];
    int n[3];
    int added = 0;
    int ix;
    int iy;
    int iz;

    if (spacing <= 0.0f) {
        return -1;
    }
    set_spacing(b, spacing);
    for (ix = 0; ix < 3; ix++) {
        ext[ix] = hi[ix] - lo[ix];
        if (ext[ix] <= 0.0f) {
            return -1;
        }
        n[ix] = (int)(ext[ix] / spacing);
        if (n[ix] < 1) {
            n[ix] = 1;
        }
    }
    if (brine_reserve(b, b->count + (size_t)n[0] * (size_t)n[1] *
                                 (size_t)n[2]) != 0) {
        return -1;
    }
    for (iz = 0; iz < n[2]; iz++) {
        for (iy = 0; iy < n[1]; iy++) {
            for (ix = 0; ix < n[0]; ix++) {
                push_particle(b, v3(lo[0] + ((float)ix + 0.5f) * spacing,
                                    lo[1] + ((float)iy + 0.5f) * spacing,
                                    lo[2] + ((float)iz + 0.5f) * spacing));
                added++;
            }
        }
    }
    return added;
}

int brine_emit(Brine *b, const float *xyz, size_t n, float spacing)
{
    size_t i;

    if (spacing <= 0.0f || n == 0 || xyz == NULL) {
        return -1;
    }
    if (n > (size_t)INT_MAX) {
        return -1;
    }
    set_spacing(b, spacing);
    if (brine_reserve(b, b->count + n) != 0) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        push_particle(b, v3(xyz[i * 3 + 0], xyz[i * 3 + 1], xyz[i * 3 + 2]));
    }
    return (int)n;
}

void brine_step(Brine *b, float dt)
{
    size_t i;
    int it;
    float inv_dt;

    if (b->count == 0 || dt <= 0.0f) {
        return;
    }

    for (i = 0; i < b->count; i++) {
        Vec3 v = b->vel[i];
        v.y -= b->s.gravity * dt;
        b->vel[i] = v;
        b->pred[i] = v3_add(b->pos[i], v3_mul(v, dt));
    }

    grid_build(b);

    if (b->s.surface_tension > 0.0f) {
        kernel_cohesion(b, dt);
    }
    if (b->s.vorticity > 0.0f) {
        kernel_vorticity(b, dt);
    }
    if (b->s.surface_tension > 0.0f || b->s.vorticity > 0.0f) {
        for (i = 0; i < b->count; i++) {
            b->pred[i] = v3_add(b->pos[i], v3_mul(b->vel[i], dt));
        }
    }

    for (it = 0; it < b->s.iterations; it++) {
        kernel_density(b);
        kernel_delta(b);
    }

    inv_dt = 1.0f / dt;
    for (i = 0; i < b->count; i++) {
        b->vel[i] = v3_mul(v3_sub(b->pred[i], b->pos[i]), inv_dt);
    }

    if (b->s.viscosity > 0.0f) {
        kernel_viscosity(b);
    }

    if (b->s.damping > 0.0f) {
        float keep = 1.0f - b->s.damping * dt;
        for (i = 0; i < b->count; i++) {
            b->vel[i] = v3_mul(b->vel[i], keep);
        }
    }

    bounds_apply(b);

    for (i = 0; i < b->count; i++) {
        b->pos[i] = b->pred[i];
    }

    if (b->s.secondary) {
        foam_emit(b, dt);
        foam_advance(b, dt);
    }
}

size_t brine_count(const Brine *b)
{
    return b->count;
}

const char *brine_version(void)
{
    return BRINE_VERSION;
}

const float *brine_positions(const Brine *b)
{
    return (const float *)b->pos;
}

const float *brine_velocities(const Brine *b)
{
    return (const float *)b->vel;
}

float brine_density(const Brine *b, size_t i)
{
    if (i >= b->count) {
        return 0.0f;
    }
    return b->rho[i];
}

float brine_density_at(const Brine *b, const float p[3])
{
    Vec3 q = v3(p[0], p[1], p[2]);
    float h2 = b->h2;
    float coef6 = b->coef6;
    float mass = b->s.mass;
    float density = 0.0f;
    size_t i;

    for (i = 0; i < b->count; i++) {
        Vec3 r = v3_sub(q, b->pos[i]);
        float r2 = v3_dot(r, r);
        if (r2 < h2) {
            float d = h2 - r2;
            density += mass * coef6 * d * d * d;
        }
    }
    return density;
}

void brine_apply_force_local(Brine *b, float dt, const Vec3 p, float r,
                            const Vec3 f)
{
    size_t i;
    float rmax = r * r;

    if (r <= 0.0f) {
        return;
    }
    for (i = 0; i < b->count; i++) {
        Vec3 dp = v3_sub(b->pos[i], p);
        float dist2 = v3_dot(dp, dp);
        if (dist2 < rmax) {
            float t = 1.0f - dist2 / rmax;
            b->vel[i] = v3_add(b->vel[i], v3_mul(f, dt * t));
        }
    }
}

void brine_apply_force(Brine *b, float dt, const float p[3], float r,
                       const float f[3])
{
    Vec3 vp = v3(p[0], p[1], p[2]);
    Vec3 vf = v3(f[0], f[1], f[2]);

    brine_apply_force_local(b, dt, vp, r, vf);
}

size_t brine_secondary_count(const Brine *b)
{
    return b->scount;
}

const float *brine_secondary_positions(const Brine *b)
{
    return (const float *)b->spos;
}

const float *brine_secondary_ages(const Brine *b)
{
    return b->sage;
}

const unsigned char *brine_secondary_types(const Brine *b)
{
    return b->stype;
}

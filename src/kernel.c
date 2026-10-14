#include "brine_internal.h"

#include <math.h>

#ifdef _OPENMP
#define BRINE_OMP _Pragma("omp parallel for schedule(static)")
#else
#define BRINE_OMP
#endif

static const float PI = 3.14159265358979f;
static const float CORR_K = 0.0003f;
static const float CORR_DQ = 0.2f;

void kernel_constants(Brine *b)
{
    float h = b->s.h;
    float dq = CORR_DQ * h;
    float d0 = h * h - dq * dq;

    b->h2 = h * h;
    b->inv_h = 1.0f / h;
    b->coef6 = 315.0f / (64.0f * PI * powf(h, 9.0f));
    b->coefs = -45.0f / (PI * powf(h, 6.0f));
    b->inv_rho0 = 1.0f / b->s.rest_density;
    b->corr_dq = 1.0f / (d0 * d0 * d0);
    b->cohesion_coef = 32.0f / (PI * powf(h, 9.0f));
}

void kernel_density(Brine *b)
{
    int n = (int)b->count;
    int gx = b->gx;
    int gy = b->gy;
    int gz = b->gz;
    float h = b->s.h;
    float h2 = b->h2;
    float coef6 = b->coef6;
    float coefs = b->coefs;
    float mass = b->s.mass;
    float inv_rho0 = b->inv_rho0;
    float epsilon = b->s.epsilon;
    int i;

    BRINE_OMP
    for (i = 0; i < n; i++) {
        Vec3 pi = b->pred[i];
        float density = mass * coef6 * h2 * h2 * h2;
        float sumgrad = 0.0f;
        int c = b->cell_of[i];
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
                        Vec3 r;
                        float r2;
                        if (j == i) {
                            continue;
                        }
                        r = v3_sub(pi, b->pred[j]);
                        r2 = v3_dot(r, r);
                        if (r2 < h2) {
                            float d = h2 - r2;
                            float rm;
                            float w;
                            density += mass * coef6 * d * d * d;
                            rm = sqrtf(r2);
                            w = coefs * (h - rm) * (h - rm);
                            sumgrad += w * w;
                        }
                    }
                }
            }
        }
        b->rho[i] = density;
        b->lambda[i] = -(density * inv_rho0 - 1.0f) /
                       (sumgrad * inv_rho0 * inv_rho0 + epsilon);
    }
}

void kernel_delta(Brine *b)
{
    int n = (int)b->count;
    int gx = b->gx;
    int gy = b->gy;
    int gz = b->gz;
    float h = b->s.h;
    float h2 = b->h2;
    float coefs = b->coefs;
    float inv_rho0 = b->inv_rho0;
    float corr_dq = b->corr_dq;
    int i;

    BRINE_OMP
    for (i = 0; i < n; i++) {
        Vec3 pi = b->pred[i];
        Vec3 dp = v3(0.0f, 0.0f, 0.0f);
        int c = b->cell_of[i];
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
                        Vec3 r;
                        float r2;
                        float rm;
                        float ratio;
                        float corr;
                        float scorr;
                        float w;
                        float scale;
                        if (j == i) {
                            continue;
                        }
                        r = v3_sub(pi, b->pred[j]);
                        r2 = v3_dot(r, r);
                        if (r2 >= h2) {
                            continue;
                        }
                        rm = sqrtf(r2);
                        if (rm < 1.0e-6f) {
                            continue;
                        }
                        ratio = h2 - r2;
                        ratio = ratio * ratio * ratio * corr_dq;
                        corr = ratio * ratio;
                        corr = corr * corr;
                        scorr = -CORR_K * corr;
                        w = coefs * (h - rm) * (h - rm);
                        scale = (b->lambda[i] + b->lambda[j] + scorr) * (w / rm);
                        dp = v3_add(dp, v3_mul(r, scale));
                    }
                }
            }
        }
        b->delta[i] = v3_mul(dp, inv_rho0);
    }

    BRINE_OMP
    for (i = 0; i < n; i++) {
        b->pred[i] = v3_add(b->pred[i], b->delta[i]);
    }
}

void kernel_viscosity(Brine *b)
{
    int n = (int)b->count;
    int gx = b->gx;
    int gy = b->gy;
    int gz = b->gz;
    float h2 = b->h2;
    float coef6 = b->coef6;
    float eps = b->s.viscosity;
    int i;

    BRINE_OMP
    for (i = 0; i < n; i++) {
        Vec3 pi = b->pred[i];
        Vec3 vi = b->vel[i];
        Vec3 dv = v3(0.0f, 0.0f, 0.0f);
        float wsum = 0.0f;
        int c = b->cell_of[i];
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
                        Vec3 r;
                        float r2;
                        if (j == i) {
                            continue;
                        }
                        r = v3_sub(pi, b->pred[j]);
                        r2 = v3_dot(r, r);
                        if (r2 < h2) {
                            float d = h2 - r2;
                            float w = coef6 * d * d * d;
                            dv = v3_add(dv, v3_mul(v3_sub(b->vel[j], vi), w));
                            wsum += w;
                        }
                    }
                }
            }
        }
        if (wsum > 0.0f) {
            b->delta[i] = v3_mul(dv, eps / wsum);
        } else {
            b->delta[i] = v3(0.0f, 0.0f, 0.0f);
        }
    }

    BRINE_OMP
    for (i = 0; i < n; i++) {
        b->vel[i] = v3_add(b->vel[i], b->delta[i]);
    }
}

void kernel_cohesion(Brine *b, float dt)
{
    int n = (int)b->count;
    int gx = b->gx;
    int gy = b->gy;
    int gz = b->gz;
    float h = b->s.h;
    float h2 = b->h2;
    float half = 0.5f * h;
    float coef = b->cohesion_coef;
    float h6_64 = powf(h, 6.0f) / 64.0f;
    float gamma = b->s.surface_tension * b->s.mass;
    int i;

    BRINE_OMP
    for (i = 0; i < n; i++) {
        Vec3 pi = b->pred[i];
        Vec3 acc = v3(0.0f, 0.0f, 0.0f);
        int c = b->cell_of[i];
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
                        Vec3 r;
                        float r2;
                        float rm;
                        float hr;
                        float c3;
                        float cval;
                        if (j == i) {
                            continue;
                        }
                        r = v3_sub(pi, b->pred[j]);
                        r2 = v3_dot(r, r);
                        if (r2 >= h2 || r2 <= 0.0f) {
                            continue;
                        }
                        rm = sqrtf(r2);
                        hr = h - rm;
                        c3 = hr * hr * hr * rm * rm * rm;
                        if (rm <= half) {
                            cval = coef * (2.0f * c3 - h6_64);
                        } else {
                            cval = coef * c3;
                        }
                        acc = v3_add(acc, v3_mul(r, -gamma * cval / rm));
                    }
                }
            }
        }
        b->vel[i] = v3_add(b->vel[i], v3_mul(acc, dt));
    }
}

void kernel_vorticity(Brine *b, float dt)
{
    int n = (int)b->count;
    int gx = b->gx;
    int gy = b->gy;
    int gz = b->gz;
    float h = b->s.h;
    float h2 = b->h2;
    float coefs = b->coefs;
    float eps = b->s.vorticity;
    float vol = b->s.mass * b->inv_rho0;
    int i;

    BRINE_OMP
    for (i = 0; i < n; i++) {
        Vec3 pi = b->pred[i];
        Vec3 vi = b->vel[i];
        Vec3 curl = v3(0.0f, 0.0f, 0.0f);
        int c = b->cell_of[i];
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
                        Vec3 r;
                        float r2;
                        float rm;
                        float g;
                        if (j == i) {
                            continue;
                        }
                        r = v3_sub(pi, b->pred[j]);
                        r2 = v3_dot(r, r);
                        if (r2 >= h2 || r2 <= 0.0f) {
                            continue;
                        }
                        rm = sqrtf(r2);
                        g = coefs * (h - rm) * (h - rm) / rm;
                        curl = v3_add(curl, v3_cross(v3_sub(b->vel[j], vi),
                                                     v3_mul(r, g * vol)));
                    }
                }
            }
        }
        b->omega[i] = curl;
        b->omega_mag[i] = v3_len(curl);
    }

    BRINE_OMP
    for (i = 0; i < n; i++) {
        Vec3 pi = b->pred[i];
        Vec3 eta = v3(0.0f, 0.0f, 0.0f);
        float elen;
        Vec3 force;
        int c = b->cell_of[i];
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
                        Vec3 r;
                        float r2;
                        float rm;
                        float g;
                        if (j == i) {
                            continue;
                        }
                        r = v3_sub(pi, b->pred[j]);
                        r2 = v3_dot(r, r);
                        if (r2 >= h2 || r2 <= 0.0f) {
                            continue;
                        }
                        rm = sqrtf(r2);
                        g = coefs * (h - rm) * (h - rm) / rm;
                        eta = v3_add(eta, v3_mul(r, g * b->omega_mag[j] * vol));
                    }
                }
            }
        }
        elen = v3_len(eta);
        if (elen < 1.0e-6f) {
            continue;
        }
        eta = v3_mul(eta, 1.0f / elen);
        force = v3_cross(eta, b->omega[i]);
        b->vel[i] = v3_add(b->vel[i], v3_mul(force, eps * dt));
    }
}

#ifndef BRINE_INTERNAL_H
#define BRINE_INTERNAL_H

#include "brine.h"
#include "vec3.h"

#include <stddef.h>

typedef struct {
    float gravity;
    float rest_density;
    float h;
    float mass;
    float epsilon;
    float viscosity;
    float restitution;
    float damping;
    float surface_tension;
    float vorticity;
    float spacing;
    int iterations;
    Vec3 bmin;
    Vec3 bmax;
} Settings;

struct Brine {
    Settings s;

    Vec3 *pos;
    Vec3 *vel;
    Vec3 *pred;
    Vec3 *delta;
    Vec3 *omega;
    float *rho;
    float *lambda;
    float *omega_mag;
    size_t count;
    size_t cap;

    Vec3 grid_min;
    float inv_h;
    float h2;
    float coef6;
    float coefs;
    float inv_rho0;
    float corr_dq;
    float cohesion_coef;
    int gx;
    int gy;
    int gz;
    int ncells;

    int *cell_count;
    int *cell_start;
    int *cursor;
    int *cell_of;
    int *order;
    int cell_cap;
    int order_cap;
};

int brine_reserve(Brine *b, size_t n);

void grid_build(Brine *b);
int grid_cell(const Brine *b, Vec3 p);

void kernel_constants(Brine *b);
void kernel_density(Brine *b);
void kernel_delta(Brine *b);
void kernel_viscosity(Brine *b);
void kernel_cohesion(Brine *b, float dt);
void kernel_vorticity(Brine *b, float dt);

void bounds_apply(Brine *b);

#endif

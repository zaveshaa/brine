#ifndef BRINE_H
#define BRINE_H

#include <stddef.h>

#define BRINE_VERSION_MAJOR 2
#define BRINE_VERSION_MINOR 0
#define BRINE_VERSION_PATCH 0
#define BRINE_VERSION "2.0.0"

typedef struct Brine Brine;

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
    int iterations;
    float bounds_min[3];
    float bounds_max[3];
} BrineParams;

BrineParams brine_default_params(void);

Brine *brine_new(const BrineParams *p);
void brine_free(Brine *b);
void brine_clear(Brine *b);

int brine_emit_box(Brine *b, const float lo[3], const float hi[3], float spacing);

size_t brine_count(const Brine *b);
const char *brine_version(void);

void brine_step(Brine *b, float dt);

const float *brine_positions(const Brine *b);
const float *brine_velocities(const Brine *b);

float brine_density(const Brine *b, size_t i);
float brine_density_at(const Brine *b, const float p[3]);

#endif

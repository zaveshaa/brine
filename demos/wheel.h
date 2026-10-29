#ifndef BRINE_WHEEL_H
#define BRINE_WHEEL_H

#include <stddef.h>

typedef struct {
    float cx;
    float cy;
    float cz;
    float radius;
    float blade_t;
    float blade_z;
    int blades;
    float theta;
    float omega;
} Wheel;

void wheel_init(Wheel *w);
float wheel_bound(const Wheel *w);
void wheel_step(Wheel *w, float dt);
int wheel_contains(const Wheel *w, const float p[3], float pr);
size_t wheel_collide(const Wheel *w, float *pos, float *vel, size_t n,
                     float pr, float restitution, float grip);

#endif

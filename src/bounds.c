#include "brine_internal.h"

void bounds_apply(Brine *b)
{
    float e = b->s.restitution;
    float margin = b->s.spacing * 0.5f;
    Vec3 lo = v3(b->s.bmin.x + margin, b->s.bmin.y + margin, b->s.bmin.z + margin);
    Vec3 hi = v3(b->s.bmax.x - margin, b->s.bmax.y - margin, b->s.bmax.z - margin);
    size_t i;

    for (i = 0; i < b->count; i++) {
        Vec3 p = b->pred[i];
        Vec3 v = b->vel[i];

        if (p.x < lo.x) {
            p.x = lo.x;
            if (v.x < 0.0f) {
                v.x = -v.x * e;
            }
        } else if (p.x > hi.x) {
            p.x = hi.x;
            if (v.x > 0.0f) {
                v.x = -v.x * e;
            }
        }
        if (p.y < lo.y) {
            p.y = lo.y;
            if (v.y < 0.0f) {
                v.y = -v.y * e;
            }
        } else if (p.y > hi.y) {
            p.y = hi.y;
            if (v.y > 0.0f) {
                v.y = -v.y * e;
            }
        }
        if (p.z < lo.z) {
            p.z = lo.z;
            if (v.z < 0.0f) {
                v.z = -v.z * e;
            }
        } else if (p.z > hi.z) {
            p.z = hi.z;
            if (v.z > 0.0f) {
                v.z = -v.z * e;
            }
        }
        b->pred[i] = p;
        b->vel[i] = v;
    }
}

#include "wheel.h"

#include <math.h>

static const float PI = 3.14159265358979f;

void wheel_init(Wheel *w)
{
    w->cx = 0.0f;
    w->cy = 0.19f;
    w->cz = 0.0f;
    w->radius = 0.135f;
    w->blade_t = 0.024f;
    w->blade_z = 0.070f;
    w->blades = 6;
    w->theta = 0.0f;
    w->omega = 2.0f;
}

float wheel_bound(const Wheel *w)
{
    return w->radius + 0.5f * ((w->blade_t > w->blade_z) ? w->blade_t
                                                         : w->blade_z);
}

void wheel_step(Wheel *w, float dt)
{
    w->theta += w->omega * dt;
    if (w->theta > 64.0f || w->theta < -64.0f) {
        w->theta = fmodf(w->theta, 2.0f * PI);
    }
}

static int blade_probe(const Wheel *w, int k, float dx, float dy, float dz,
                       float pr, float c[3], float n[3], float *dist)
{
    float a = w->theta + 2.0f * PI / (float)w->blades * (float)k;
    float ca = cosf(a);
    float sa = sinf(a);
    float hx = 0.5f * w->radius;
    float hy = 0.5f * w->blade_t;
    float hz = 0.5f * w->blade_z;
    float rl = dx * ca + dy * sa;
    float tl = -dx * sa + dy * ca;
    float qx = rl - hx;
    float bx = fmaxf(-hx, fminf(hx, qx));
    float by = fmaxf(-hy, fminf(hy, tl));
    float bz = fmaxf(-hz, fminf(hz, dz));
    float ex = qx - bx;
    float ey = tl - by;
    float ez = dz - bz;
    float d2 = ex * ex + ey * ey + ez * ez;

    if (d2 >= pr * pr) {
        return 0;
    }
    if (d2 > 1.0e-12f) {
        float d = sqrtf(d2);

        n[0] = ex / d;
        n[1] = ey / d;
        n[2] = ez / d;
        *dist = d;
    } else {
        float px = hx - fabsf(qx);
        float py = hy - fabsf(tl);
        float pz = hz - fabsf(dz);

        if (px <= py && px <= pz) {
            n[0] = (qx >= 0.0f) ? 1.0f : -1.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
            *dist = -px;
        } else if (py <= pz) {
            n[0] = 0.0f;
            n[1] = (tl >= 0.0f) ? 1.0f : -1.0f;
            n[2] = 0.0f;
            *dist = -py;
        } else {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = (dz >= 0.0f) ? 1.0f : -1.0f;
            *dist = -pz;
        }
    }
    c[0] = bx + hx;
    c[1] = by;
    c[2] = bz;
    return 1;
}

int wheel_contains(const Wheel *w, const float p[3], float pr)
{
    float dx = p[0] - w->cx;
    float dy = p[1] - w->cy;
    float dz = p[2] - w->cz;
    int k;

    if (pr <= 0.0f || w->blades < 1) {
        return 0;
    }
    if ((dx * dx + dy * dy + dz * dz) >
        (wheel_bound(w) + pr) * (wheel_bound(w) + pr)) {
        return 0;
    }
    for (k = 0; k < w->blades; k++) {
        float c[3];
        float n[3];
        float dist;

        if (blade_probe(w, k, dx, dy, dz, pr, c, n, &dist)) {
            return 1;
        }
    }
    return 0;
}

static void contact(const Wheel *w, float *pos, float *vel, float pen,
                    float nx, float ny, float nz, float qx, float qy,
                    float restitution, float grip)
{
    float sx = -w->omega * (qy - w->cy);
    float sy = w->omega * (qx - w->cx);
    float rx;
    float ry;
    float rz;
    float vn;

    pos[0] += pen * nx;
    pos[1] += pen * ny;
    pos[2] += pen * nz;

    rx = vel[0] - sx;
    ry = vel[1] - sy;
    rz = vel[2];
    vn = rx * nx + ry * ny + rz * nz;
    if (vn < 0.0f) {
        float j = -(1.0f + restitution) * vn;
        float tx = rx - vn * nx;
        float ty = ry - vn * ny;
        float tz = rz - vn * nz;

        vel[0] += j * nx;
        vel[1] += j * ny;
        vel[2] += j * nz;
        vel[0] -= grip * tx;
        vel[1] -= grip * ty;
        vel[2] -= grip * tz;
    }
}

size_t wheel_collide(const Wheel *w, float *pos, float *vel, size_t n,
                     float pr, float restitution, float grip)
{
    float bound = wheel_bound(w) + pr;
    float bound2 = bound * bound;
    size_t i;
    size_t hits = 0;

    if (pr <= 0.0f || w->blades < 1 || grip < 0.0f || grip > 1.0f) {
        return 0;
    }

    for (i = 0; i < n; i++) {
        float dx = pos[i * 3 + 0] - w->cx;
        float dy = pos[i * 3 + 1] - w->cy;
        float dz = pos[i * 3 + 2] - w->cz;
        int k;

        if (dx * dx + dy * dy + dz * dz > bound2) {
            continue;
        }

        for (k = 0; k < w->blades; k++) {
            float cl[3];
            float nl[3];
            float dist;
            float a;
            float ca;
            float sa;
            float qx;
            float qy;

            if (!blade_probe(w, k, dx, dy, dz, pr, cl, nl, &dist)) {
                continue;
            }
            a = w->theta + 2.0f * PI / (float)w->blades * (float)k;
            ca = cosf(a);
            sa = sinf(a);
            qx = cl[0] * ca - cl[1] * sa;
            qy = cl[0] * sa + cl[1] * ca;
            contact(w, pos + i * 3, vel + i * 3, pr - dist,
                    nl[0] * ca - nl[1] * sa, nl[0] * sa + nl[1] * ca, nl[2],
                    w->cx + qx, w->cy + qy, restitution, grip);
            hits++;
        }
    }
    return hits;
}

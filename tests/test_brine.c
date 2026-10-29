#include "brine.h"
#include "wheel.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond, msg)                    \
    do {                                    \
        if (!(cond)) {                      \
            printf("FAIL: %s\n", (msg));    \
            failures++;                     \
        }                                   \
    } while (0)

static int same(const float *a, const float *b, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    BrineParams p = brine_default_params();
    float lo[3] = {-0.30f, 1.00f, -0.30f};
    float hi[3] = {0.30f, 1.60f, 0.30f};
    Brine *a;
    Brine *b;
    int added;
    size_t n;
    const float *pos;
    size_t i;
    int step;

    a = brine_new(&p);
    CHECK(a != NULL, "world is created");
    added = brine_emit_box(a, lo, hi, 0.08f);
    CHECK(added > 0, "emit_box adds particles");
    CHECK(brine_count(a) == (size_t)added, "count matches what was added");
    n = brine_count(a);

    b = brine_new(&p);
    brine_emit_box(b, lo, hi, 0.08f);
    for (step = 0; step < 400; step++) {
        brine_step(a, 1.0f / 120.0f);
        brine_step(b, 1.0f / 120.0f);
    }

    CHECK(same(brine_positions(a), brine_positions(b), n * 3),
          "two worlds stay bit-identical (deterministic)");

    pos = brine_positions(a);
    for (i = 0; i < n; i++) {
        float x = pos[i * 3 + 0];
        float y = pos[i * 3 + 1];
        float z = pos[i * 3 + 2];
        CHECK(x >= p.bounds_min[0] - 1e-4f && x <= p.bounds_max[0] + 1e-4f,
              "particle x inside the box");
        CHECK(y >= p.bounds_min[1] - 1e-4f && y <= p.bounds_max[1] + 1e-4f,
              "particle y inside the box");
        CHECK(z >= p.bounds_min[2] - 1e-4f && z <= p.bounds_max[2] + 1e-4f,
              "particle z inside the box");
        CHECK(!isnan(x) && !isnan(y) && !isnan(z), "no NaN in positions");
    }

    {
        float q[3] = {pos[0], pos[1], pos[2]};
        CHECK(brine_density_at(a, q) > 0.0f, "density at a particle is positive");
    }

    CHECK(strcmp(brine_version(), BRINE_VERSION) == 0, "version string matches");

    {
        BrineParams q = brine_default_params();
        Brine *c;
        Brine *d;
        size_t nc;
        const float *sp;

        q.secondary = 1;
        c = brine_new(&q);
        d = brine_new(&q);
        brine_emit_box(c, lo, hi, 0.08f);
        brine_emit_box(d, lo, hi, 0.08f);
        for (step = 0; step < 400; step++) {
            brine_step(c, 1.0f / 120.0f);
            brine_step(d, 1.0f / 120.0f);
        }
        nc = brine_secondary_count(c);
        CHECK(nc > 0, "secondary particles are generated");
        CHECK(nc <= brine_count(c), "secondary count stays within budget");
        sp = brine_secondary_positions(c);
        for (i = 0; i < nc * 3; i++) {
            CHECK(!isnan(sp[i]), "no NaN in secondary positions");
        }
        CHECK(brine_secondary_count(d) == nc,
              "secondary count is deterministic");
        CHECK(same(sp, brine_secondary_positions(d), nc * 3),
              "secondaries stay bit-identical (deterministic)");
        brine_free(c);
        brine_free(d);
    }

    {
        Wheel w;
        float q[3];
        float v[3];

        wheel_init(&w);
        w.theta = 0.0f;
        w.omega = 0.0f;
        q[0] = w.cx + 0.06f;
        q[1] = w.cy;
        q[2] = w.cz;
        v[0] = v[1] = v[2] = 0.0f;
        CHECK(wheel_collide(&w, q, v, 1, 0.0275f, 0.05f, 0.45f) > 0,
              "blade touches an embedded particle");
        CHECK(fabsf(q[1] - w.cy) >= 0.0275f + 0.5f * w.blade_t - 1.0e-4f,
              "blade pushes the particle out of its body");

        wheel_init(&w);
        w.theta = 0.0f;
        w.omega = 2.0f;
        q[0] = w.cx + 0.06f;
        q[1] = w.cy + 0.0275f + 0.5f * w.blade_t - 0.004f;
        q[2] = w.cz;
        v[0] = v[1] = v[2] = 0.0f;
        wheel_collide(&w, q, v, 1, 0.0275f, 0.05f, 0.45f);
        CHECK(v[1] > 0.05f, "a spinning blade drags the particle along");
        CHECK(q[1] > w.cy, "the particle is pushed clear of the blade face");

        wheel_init(&w);
        w.omega = 3.0f;
        for (step = 0; step < 600; step++) {
            wheel_step(&w, 1.0f / 120.0f);
        }
        CHECK(isfinite(w.theta), "wheel angle stays finite");
    }

    {
        Brine *e = brine_new(&p);
        float xyz[9] = {0.1f, 0.2f, 0.3f, -0.4f, 0.5f, 0.6f, 0.0f, 0.0f, 0.0f};
        const float *q;

        CHECK(e != NULL, "world for emit is created");
        CHECK(brine_emit(e, xyz, 3, 0.05f) == 3, "emit adds the given particles");
        CHECK(brine_count(e) == 3, "emit count matches the batch");
        q = brine_positions(e);
        CHECK(q[0] == 0.1f && q[4] == 0.5f && q[8] == 0.0f,
              "emit keeps the positions it was given");
        CHECK(brine_density_at(e, xyz) > 0.0f,
              "emit derives the particle mass from the spacing");
        CHECK(brine_emit(e, xyz, 0, 0.05f) < 0, "emit rejects an empty batch");
        CHECK(brine_emit(e, xyz, 3, 0.0f) < 0, "emit rejects a bad spacing");
        brine_free(e);
    }

    {
        Wheel w;
        float q[3];
        float v[3];
        int mismatch = 0;
        int ix;
        int iy;
        int iz;

        wheel_init(&w);
        w.omega = 0.0f;
        w.theta = 0.35f;
        for (iz = -4; iz <= 4; iz++) {
            for (iy = -6; iy <= 6; iy++) {
                for (ix = -8; ix <= 8; ix++) {
                    int want;
                    int got;

                    q[0] = w.cx + (float)ix * 0.03f;
                    q[1] = w.cy + (float)iy * 0.03f;
                    q[2] = w.cz + (float)iz * 0.02f;
                    v[0] = v[1] = v[2] = 0.0f;
                    want = wheel_contains(&w, q, 0.0275f);
                    got = wheel_collide(&w, q, v, 1, 0.0275f, 0.05f, 0.0f) > 0;
                    if (want != got) {
                        mismatch++;
                    }
                }
            }
        }
        CHECK(mismatch == 0, "wheel_contains agrees with wheel_collide");
    }

    brine_free(a);
    brine_free(b);

    if (failures != 0) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("brine: all checks passed (%zu particles)\n", n);
    return 0;
}

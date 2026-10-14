#include "brine.h"

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

    brine_free(a);
    brine_free(b);

    if (failures != 0) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("brine: all checks passed (%zu particles)\n", n);
    return 0;
}

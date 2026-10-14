#include "brine.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#define COLS 72
#define ROWS 22
#define ZSAMPLES 20

static const char RAMP[] = " .:-=+*#%@";

#define XMIN (-0.6f)
#define XMAX (0.6f)
#define YMIN (0.0f)
#define YMAX (1.4f)
#define ZMIN (-0.6f)
#define ZMAX (0.6f)

static char cell(Brine *b, float x, float y, float rest)
{
    float best = 0.0f;
    int zs;
    int last = (int)(sizeof RAMP) - 2;
    int idx;

    for (zs = 0; zs < ZSAMPLES; zs++) {
        float z = ZMIN + ((float)zs + 0.5f) * (ZMAX - ZMIN) / (float)ZSAMPLES;
        float pos[3] = {x, y, z};
        float d = brine_density_at(b, pos);
        if (d > best) {
            best = d;
        }
    }
    idx = (int)((best / rest) * (float)last);
    if (idx < 0) {
        idx = 0;
    } else if (idx > last) {
        idx = last;
    }
    return RAMP[idx];
}

int main(void)
{
    BrineParams p = brine_default_params();
    Brine *b;
    float lo[3] = {-0.45f, 0.80f, -0.45f};
    float hi[3] = {0.45f, 1.35f, 0.45f};
    int step;
    int r;
    int c;

    p.bounds_min[0] = XMIN;
    p.bounds_max[0] = XMAX;
    p.bounds_min[1] = YMIN;
    p.bounds_max[1] = YMAX;
    p.bounds_min[2] = ZMIN;
    p.bounds_max[2] = ZMAX;
    p.iterations = 12;

    b = brine_new(&p);
    if (b == NULL) {
        fprintf(stderr, "brine: out of memory\n");
        return 1;
    }
    brine_emit_box(b, lo, hi, 0.08f);

    for (step = 0; step < 1500; step++) {
        brine_step(b, 1.0f / 120.0f);
    }

    printf("brine %s - %zu particles, %d steps\n\n",
           brine_version(), brine_count(b), step);

    for (c = 0; c < COLS + 2; c++) {
        putchar(c == 0 || c == COLS + 1 ? '+' : '-');
    }
    putchar('\n');

    for (r = 0; r < ROWS; r++) {
        float y = YMAX - ((float)r + 0.5f) * (YMAX - YMIN) / (float)ROWS;
        putchar('|');
        for (c = 0; c < COLS; c++) {
            float x = XMIN + ((float)c + 0.5f) * (XMAX - XMIN) / (float)COLS;
            putchar(cell(b, x, y, p.rest_density));
        }
        putchar('|');
        putchar('\n');
    }

    for (c = 0; c < COLS + 2; c++) {
        putchar(c == 0 || c == COLS + 1 ? '+' : '-');
    }
    putchar('\n');

    brine_free(b);
    return 0;
}

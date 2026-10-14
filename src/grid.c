#include "brine_internal.h"

#include <stdlib.h>
#include <string.h>

static int grow_to(int need, int cap)
{
    int want = (cap != 0) ? cap : 64;

    if (need <= cap) {
        return cap;
    }
    while (want < need) {
        want *= 2;
    }
    return want;
}

int grid_cell(const Brine *b, Vec3 p)
{
    int ix = (int)((p.x - b->grid_min.x) * b->inv_h);
    int iy = (int)((p.y - b->grid_min.y) * b->inv_h);
    int iz = (int)((p.z - b->grid_min.z) * b->inv_h);

    if (ix < 0) {
        ix = 0;
    } else if (ix >= b->gx) {
        ix = b->gx - 1;
    }
    if (iy < 0) {
        iy = 0;
    } else if (iy >= b->gy) {
        iy = b->gy - 1;
    }
    if (iz < 0) {
        iz = 0;
    } else if (iz >= b->gz) {
        iz = b->gz - 1;
    }
    return (iz * b->gy + iy) * b->gx + ix;
}

static int reserve_cells(Brine *b, int need)
{
    int want;
    int *a;
    int *s;
    int *cur;

    if (need <= b->cell_cap) {
        return 0;
    }
    want = grow_to(need, b->cell_cap);
    a = realloc(b->cell_count, (size_t)want * sizeof *a);
    if (a == NULL) {
        return -1;
    }
    b->cell_count = a;
    s = realloc(b->cell_start, (size_t)want * sizeof *s);
    if (s == NULL) {
        return -1;
    }
    b->cell_start = s;
    cur = realloc(b->cursor, (size_t)want * sizeof *cur);
    if (cur == NULL) {
        return -1;
    }
    b->cursor = cur;
    b->cell_cap = want;
    return 0;
}

static int reserve_order(Brine *b, int need)
{
    int want;
    int *co;
    int *o;

    if (need <= b->order_cap) {
        return 0;
    }
    want = grow_to(need, b->order_cap);
    co = realloc(b->cell_of, (size_t)want * sizeof *co);
    if (co == NULL) {
        return -1;
    }
    b->cell_of = co;
    o = realloc(b->order, (size_t)want * sizeof *o);
    if (o == NULL) {
        return -1;
    }
    b->order = o;
    b->order_cap = want;
    return 0;
}

void grid_build(Brine *b)
{
    int n = (int)b->count;
    int c;
    int i;

    b->grid_min = v3(b->s.bmin.x - b->s.h, b->s.bmin.y - b->s.h,
                     b->s.bmin.z - b->s.h);
    b->gx = (int)((b->s.bmax.x - b->grid_min.x) * b->inv_h) + 1;
    b->gy = (int)((b->s.bmax.y - b->grid_min.y) * b->inv_h) + 1;
    b->gz = (int)((b->s.bmax.z - b->grid_min.z) * b->inv_h) + 1;
    if (b->gx < 1) {
        b->gx = 1;
    }
    if (b->gy < 1) {
        b->gy = 1;
    }
    if (b->gz < 1) {
        b->gz = 1;
    }
    b->ncells = b->gx * b->gy * b->gz;

    if (reserve_cells(b, b->ncells + 1) != 0 ||
        reserve_order(b, n > 1 ? n : 1) != 0) {
        return;
    }

    memset(b->cell_count, 0, (size_t)b->ncells * sizeof *b->cell_count);
    for (i = 0; i < n; i++) {
        c = grid_cell(b, b->pred[i]);
        b->cell_of[i] = c;
        b->cell_count[c]++;
    }
    b->cell_start[0] = 0;
    for (c = 0; c < b->ncells; c++) {
        b->cell_start[c + 1] = b->cell_start[c] + b->cell_count[c];
    }
    for (c = 0; c < b->ncells; c++) {
        b->cursor[c] = b->cell_start[c];
    }
    for (i = 0; i < n; i++) {
        c = b->cell_of[i];
        b->order[b->cursor[c]++] = i;
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int w;
    int h;
    unsigned char *px;
} Image;

static int read_ppm(const char *path, Image *im)
{
    FILE *f = fopen(path, "rb");
    char magic[3];
    int maxv;

    if (f == NULL) {
        fprintf(stderr, "golden: cannot open %s\n", path);
        return -1;
    }
    if (fscanf(f, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "golden: %s is not a P6 ppm\n", path);
        fclose(f);
        return -1;
    }
    if (fscanf(f, "%d %d %d", &im->w, &im->h, &maxv) != 3 || maxv != 255) {
        fprintf(stderr, "golden: %s has a bad header\n", path);
        fclose(f);
        return -1;
    }
    {
        long n = (long)im->w * (long)im->h * 3;
        im->px = (unsigned char *)malloc((size_t)n);
        if (im->px == NULL || fread(im->px, 1, (size_t)n, f) != (size_t)n) {
            fprintf(stderr, "golden: %s is truncated\n", path);
            fclose(f);
            free(im->px);
            im->px = NULL;
            return -1;
        }
    }
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    Image a = {0, 0, NULL};
    Image b = {0, 0, NULL};
    double tol = (argc > 3) ? atof(argv[3]) : 6.0;
    long n;
    long i;
    double sum = 0.0;
    long changed = 0;
    int rc = 0;

    if (argc < 3) {
        fprintf(stderr, "usage: golden current.ppm reference.ppm [tol]\n");
        return 2;
    }
    if (read_ppm(argv[1], &a) != 0 || read_ppm(argv[2], &b) != 0) {
        free(a.px);
        free(b.px);
        return 2;
    }
    if (a.w != b.w || a.h != b.h) {
        fprintf(stderr, "golden: size mismatch %dx%d vs %dx%d\n", a.w, a.h, b.w,
                b.h);
        free(a.px);
        free(b.px);
        return 1;
    }
    n = (long)a.w * (long)a.h * 3;
    for (i = 0; i < n; i++) {
        int d = (int)a.px[i] - (int)b.px[i];
        if (d < 0) {
            d = -d;
        }
        sum += (double)d;
        if (d > 24) {
            changed++;
        }
    }
    printf("golden: mean abs diff %.2f, changed %.3f%%, tol %.1f\n",
           sum / (double)n, 100.0 * (double)changed / (double)n, tol);
    if (sum / (double)n > tol) {
        fprintf(stderr, "golden: mismatch (mean diff too high)\n");
        rc = 1;
    }
    free(a.px);
    free(b.px);
    return rc;
}

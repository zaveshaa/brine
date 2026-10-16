<p align="center">
  <img src="assets/logo.svg" width="128" height="128" alt="brine">
</p>

<h1 align="center">brine</h1>

<p align="center">
  An SPH fluid simulation in C.
</p>

<p align="center">
  <a href="https://github.com/zaveshaa/brine/actions/workflows/ci.yml"><img
     src="https://github.com/zaveshaa/brine/actions/workflows/ci.yml/badge.svg"
     alt="ci"></a>
  <a href="LICENSE"><img
     src="https://img.shields.io/badge/licence-MIT-blue.svg"
     alt="licence: MIT"></a>
  <img src="https://img.shields.io/badge/C-C11-blue.svg" alt="C11">
  <img src="https://img.shields.io/badge/dependencies-none-brightgreen.svg"
       alt="no dependencies">
</p>

## Overview

`brine` is a small library that turns a block of particles into water. You
create a world, fill a box with particles, call `brine_step` in your frame
loop, and read the particles back to draw them. The solver is position-based:
it predicts where every particle wants to move, then projects the positions
back until the fluid density matches the target. Nothing here allocates behind
your back, and nothing is random, so the same world stepped the same way gives
the same numbers on every machine.

The interface is five calls wide:

```c
#include <brine.h>

BrineParams p = brine_default_params();
Brine *w = brine_new(&p);

float lo[3] = {-0.4f, 0.8f, -0.4f};
float hi[3] = { 0.4f, 1.3f,  0.4f};
brine_emit_box(w, lo, hi, 0.08f);

for (;;) {
    brine_step(w, 1.0f / 120.0f);

    const float *xyz = brine_positions(w);
    for (size_t i = 0; i < brine_count(w); i++) {
        draw(xyz[i * 3 + 0], xyz[i * 3 + 1], xyz[i * 3 + 2]);
    }
}

brine_free(w);
```

## Build

    make            build the terminal demo
    make test       run the checks
    make run        watch a block of water settle
    make run-gl     watch it in a window (needs GLFW and an OpenGL 3.3 driver)
    make gif        record the GL demo to an animated GIF (needs ffmpeg)
    make golden     compare the GL render against tests/golden.ppm

The build is warning-clean under `-Wall -Wextra -Wpedantic -Wshadow
-Wstrict-prototypes -Wconversion`; CI builds it with every warning fatal.
There is nothing to link but the C library itself and `-lm`. The OpenGL demo
is the one extra: it links `glfw`, and nothing else, and drags in no loader
library. `demos/glapi.h` is a hand-written stub that grabs the handful of GL
3.3 entry points it needs from `glfwGetProcAddress`. The demo also builds the
solver with OpenMP when the compiler has it, so `make run-gl` fills over a
thousand particles without stalling; build with `make gl GL_OMP=` to leave it
single-threaded.

## What runs under the hood

Every step does four things, in order.

1. **Integrate.** Gravity is added to the velocity and a predicted position is
   formed for each particle.
2. **Buckets.** Particles are dropped into a uniform grid whose cell is one
   smoothing radius wide, so a particle only looks at the 27 cells around it.
3. **Project.** Density is summed with the poly6 kernel, a Lagrange multiplier
   per particle corrects the density error, and the predicted positions are
   nudged toward the rest density. This repeats for a few iterations.
4. **React.** Velocity is read back from how far the particles moved, a small
   XSPH term blends each particle's velocity toward its neighbours, and the box
   walls push back.

The solver is the position-based fluids method of Müller, Solenthaler,
Keiser and Gross (2013), including the artificial-pressure term that keeps a
free surface from clumping.

## Reading the fluid

`brine_positions` returns a packed `x y z` array, ready for a point sprite
pass. `brine_density_at` samples the density field at an arbitrary point, which
is what a ray tracer wants: march the ray and take the first place the density
turns to water, and you have a surface without any marching cubes.

Set `secondary` in the parameters and the solver also sheds spray and foam at
the surface. `brine_secondary_count`, `brine_secondary_positions`,
`brine_secondary_ages` and `brine_secondary_types` read those back; they come
as the same packed `x y z` layout as the fluid, plus a type per particle.

## Watching it move

`make run-gl` opens a window and drops around a thousand particles into a small
tub, next to a water wheel that the millrace current turns. Drag with the left
mouse button to orbit, scroll to zoom, press space to pause and `r` to reset the
view. The water is drawn with screen-space fluid rendering: every particle
splats into one shared density field, the surface is taken at an isolevel, and
the normals come from the field's gradient. A composite then refracts and
reflects the tub behind it, ripples the surface, and lays the spray and foam on
top. Set `BRINE_DUMP=out.ppm` to write a single frame and exit (with
`BRINE_DUMP_FRAME=N` to pick the frame), which is handy for grabbing a still;
`BRINE_WINDOW=WxH` sets the window size. Captures step a fixed number of
substeps per frame (`BRINE_STEPS`) so a still or a clip comes out the same every
run. `make gif` drives exactly that and encodes the frames with ffmpeg, and
`make golden` diffs a rendered frame against the stored `tests/golden.ppm` with
`tools/golden.c`.

The terminal demo `make run` draws the same solver as ASCII and needs no
window at all.

## Licence

MIT. See [LICENSE](LICENSE).

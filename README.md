<p align="center">
  <img src="assets/logo.svg" width="128" height="128" alt="brine">
</p>

<h1 align="center">brine</h1>

<p align="center">
  position-based fluid simulation in C
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

## build

```sh
make
```

`make` builds the C11 terminal demo. The core library has no dependencies;
the terminal demo needs only `-lm`. `make run-gl` additionally requires GLFW
and an OpenGL 3.3 implementation.

## run

```sh
make run       # terminal demo
make run-gl    # OpenGL demo
```

Other targets:

```sh
make test      # run the checks
make gif       # record brine.gif with ffmpeg
make golden    # compare the GL frame with tests/golden.ppm
make clean     # remove build files
```

The demos accept these environment variables:

```text
BRINE_WINDOW=WxH       window size
BRINE_STEPS=N          substeps per frame
BRINE_DUMP=out.ppm     write one frame and exit
BRINE_DUMP_FRAME=N     select a frame
BRINE_GIF=prefix       write PPM frames for make gif
```

## API

Create a world, emit particles, and advance it with fixed or variable
timesteps:

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

`brine_positions` returns packed `x y z` floats. `brine_velocities` returns
packed velocities, and `brine_density_at` evaluates the density field at an
arbitrary position.

Set `secondary` in `BrineParams` to enable spray and foam particles. Their
positions, ages, and types are available through
`brine_secondary_positions`, `brine_secondary_ages`, and
`brine_secondary_types`.

## implementation

Each step integrates gravity and predicts particle positions, inserts particles
into a uniform grid with a cell width equal to the smoothing radius, sums density
with the poly6 kernel, and solves density constraints with Lagrange multipliers.
Predicted positions are projected iteratively toward the target rest density.
Velocity is reconstructed from position change, with XSPH neighbour blending and
box-boundary response.

The solver follows the position-based fluids method of Müller, Solenthaler,
Keiser and Gross (2013), including artificial pressure. All state is stored in
the library; particle positions and densities are deterministic for a given
world and timestep sequence.

The GL demo uses screen-space fluid rendering: particles accumulate in a depth
texture, the surface is extracted at an isolevel, normals are taken from its
gradient, and a final pass composites refraction, reflection, spray, and foam.
The demo can also render the solver to PPM frames.

## layout

```text
include/    public C API
src/        solver, spatial grid, kernels, bounds, and secondary particles
demos/      terminal and OpenGL demos
assets/     logo and test assets
tests/      unit and golden-frame checks
tools/      image comparison utility
```

## license

MIT. See [LICENSE](LICENSE).

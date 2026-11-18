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

Builds the terminal demo. The library has no dependencies; the terminal demo
uses `libm`. The OpenGL demo requires GLFW and OpenGL 3.3.

## run

```sh
make run       # terminal demo
make run-gl    # OpenGL demo
```

Other targets:

```sh
make test      # tests
make gif       # record a GIF with ffmpeg
make golden    # compare a frame with tests/golden.ppm
make clean     # clean
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

```c
#include <brine.h>

BrineParams params = brine_default_params();
Brine *brine = brine_new(&params);

float lo[3] = {-0.4f, 0.8f, -0.4f};
float hi[3] = { 0.4f, 1.3f,  0.4f};
brine_emit_box(brine, lo, hi, 0.08f);

brine_step(brine, 1.0f / 120.0f);
const float *positions = brine_positions(brine);

brine_free(brine);
```

`brine_positions` and `brine_velocities` return packed `x y z` arrays.
`brine_density_at` samples the density field. Set `secondary` in
`BrineParams` to enable spray and foam particles.

## layout

```text
include/    public headers
src/        solver
demos/      terminal and OpenGL demos
assets/     logo and assets
tests/      tests
tools/      utilities
```

## license

MIT. See [LICENSE](LICENSE).

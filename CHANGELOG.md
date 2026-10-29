# Changelog

## Unreleased

- The GL demo's water wheel is now a physical body: six flat steel blades
  (plates spanning the axle), a torus rim and a spherical hub, all spinning at
  a fixed angular velocity and pushing the particles through an explicit
  contact pass (push-out along the normal, restitution, and a grip that drags
  the water the way the wheel turns). The wheel geometry lives in the demo
  (`demos/wheel.c`), not in the library.
- `brine_apply_drag` is gone from the API: nothing in the demo needs a
  velocity-matching force field any more, and a wheel is a shape, not a field.
- New `brine_emit` fills the world from an arbitrary array of positions, so the
  demo can carve its starting lattice around the wheel instead of seeding
  particles inside it (particles born inside a solid lattice explode the PBF
  projection on the first steps).
- The wheel is rendered by raymarching the same signed distance function the
  collision code uses, so what you see is exactly what the water hits.

## 2.3.0 - 2026-10-16

- The GL demo gains a water wheel: a millrace current pushes a six-bladed wheel,
  the fluid's tangential velocity at the blades spins it, and the blades push
  back into the water through `brine_apply_force`, so the wheel and the fluid
  drive each other.
- A visible sun disc with a soft glow, and a lower default camera pitch that
  keeps the sky in frame.
- The field render scale now adapts to the measured frame rate (between 0.5 and
  1.0), and the vertex buffer is filled with OpenMP when the demo is built with
  it.
- `make gif` records the GL demo to an animated GIF through ffmpeg; `make golden`
  and `make golden-update` compare the render against `tests/golden.ppm` using
  the new `tools/golden.c`.
- Capture modes (`BRINE_DUMP`, `BRINE_GIF`) step a fixed number of substeps per
  frame (`BRINE_STEPS`) and hold the field resolution fixed, so stills and clips
  reproduce across runs. `BRINE_WINDOW=WxH` sets the window size.

## 2.2.0 - 2026-10-16

- The GL demo now derives foam from particle motion (velocity) and composites it
  as an animated bubble-noise surface layer, instead of splatting the separate
  secondary particles as discrete balls.
- The per-particle foam weight is packed into the fourth vertex attribute and
  accumulated in the field's blue channel during the metaball depth pass; the
  composite pass normalises it by density and adds screen-space bubble texture.
- The secondary-particle system remains available in the library
  (`brine_secondary_*`), but is no longer enabled or drawn by the GL demo.

## 2.1.0 - 2026-10-15

- Secondary spray and foam particles (Ihmsen et al. 2012): surface particles
  with too few neighbours are shed as spray when fast and foam when slow,
  advected by the nearby fluid, and aged out.
- `secondary` parameter, `brine_secondary_count`, `brine_secondary_positions`,
  `brine_secondary_ages`, `brine_secondary_types`.
- The GL demo splats spray and foam into the field's third and fourth channels
  and composites them over the water.

## 2.0.0 - 2026-10-15

- Metaball screen-space renderer: particles splat one shared density field,
  the surface is taken at an isolevel and normals come from its gradient.
- Replaces the sphere-impostor depth pass and the bilateral-blur chain.
- Single RG32F field target; `GL_RG`, `GL_RG32F` and `GL_ONE` bindings.

## 0.1.0 - 2026-10-14

- Position-based fluid solver (Müller et al. 2013): poly6 density, spiky
  gradient, artificial pressure, XSPH viscosity, Akinci cohesion, vorticity
  confinement.
- Uniform-grid neighbour search, one cell per smoothing radius.
- Bounded domain with a half-spacing margin and wall restitution.
- `brine_emit_box`, `brine_step`, `brine_positions`, `brine_velocities`,
  `brine_density_at`.
- Terminal demo and an OpenGL 3.3 screen-space fluid demo.
- Checks for determinism, containment and finiteness; CI builds with warnings
  fatal.

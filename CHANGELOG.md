# Changelog

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

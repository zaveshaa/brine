# Changelog

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

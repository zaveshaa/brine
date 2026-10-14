# Changelog

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

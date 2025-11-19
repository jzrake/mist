# 1D Linear Advection Example

Simple example demonstrating the mist driver with a 1D linear advection equation.

## Physics

Solves: ∂u/∂t + v ∂u/∂x = 0

- First-order upwind scheme
- Periodic boundary conditions
- RK3 time integration

## Building

```bash
make
```

## Running

```bash
./advection-1d
```

## Output Files

- `chkpt.NNNN.dat` - Checkpoints (full state, every 0.5 time units)
- `prods.NNNN.dat` - Products (diagnostics, every 0.2 time units)
- `timeseries.dat` - Time series data (mass, min/max, every 0.05 time units)

## Configuration

Edit `advection-1d.cpp` and modify the `driver_config` setup:

```cpp
cfg.physics.num_zones = 200;           // Grid resolution
cfg.rk_order = 3;                      // 1, 2, or 3
cfg.cfl = 0.5;                         // CFL safety factor
cfg.t_final = 2.0;                     // End time
cfg.checkpoint_interval = 0.5;         // Checkpoint frequency
// ... etc
```

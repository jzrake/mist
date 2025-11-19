# Mist

A lightweight C++20 header-only library for deploy-anywhere array transformations and physics simulations.

Mist is an evolution of [Vapor](https://github.com/clemson-cal/vapor), a library with similar goals for computational astrophysics.

## Features

- **Core library** (`mist/core.hpp`): Multi-dimensional arrays, index spaces, transforms, and parallel execution
- **Driver library** (`mist/driver.hpp`): Time-stepping and scheduled output management for physics simulations
- **Header-only**: No compilation required, just include and go
- **CUDA compatible**: All functions work on both CPU and GPU (CUDA 12+)
- **Zero dependencies**: Pure C++20 standard library

## Quick Start

```bash
# Copy headers to your project
cp -r include/mist /path/to/your/project/

# Or clone and use directly
git clone https://github.com/yourusername/mist.git
cd mist/examples/advection-1d
make
./advection-1d
```

## CUDA Compatibility

Mist is fully compatible with CUDA 12+ and can be used in both host and device code. All functions are annotated with `__host__ __device__` when compiled with nvcc.

## Data structures
All data structures use overloaded free functions rather than member functions. Data members are public but start with an underscore.

- `vec_t<T, S>` is a statically sized array type with element type `T` and size `S`
  * aliases `dvec_t`, `ivec_t` and, `uvec_t` for types `double`, `int`, and `unsigned int`
  * constructor e.g. `vec(0.4, 1.0, 2.0) -> vec_t<double, 3>` detects `T` using `std::common_type`
  * constructors
    + `dvec`
    + `ivec`
    + `uvec`
    + `range<S> -> uvec_t<S>`
  * operators
    + `vec_t<T, S> + vec_t<U, S>`
    + `vec_t<T, S> - vec_t<U, S>`
    + `vec_t<T, S> * U` and `U * vec_t<T, S>`
    + `vec_t<T, S> / U`
    + `==`
  * free functions
    + `dot(vec<T, S>, vec<U, S>)` - dot product
    + `map(vec_t<T, S>, [] (T) -> U { ... })` - element-wise transformation
    + `sum(vec_t<T, S>)` - sum of all elements, returns `T`
    + `product(vec_t<T, S>)` - product of all elements, returns `T`
    + `any(vec_t<bool, S>)` - true if any element is true, returns `bool`
    + `all(vec_t<bool, S>)` - true if all elements are true, returns `bool`
- `index_space_t<S>` is `start: ivec_t<S>` and a `shape: uvec_t<S>`
  * constructors
    + `index_space(start, shape)` - creates an index_space_t with given start and shape
    + Example: `auto space = index_space(ivec(0, 0), uvec(10, 20));`
  * operators
    + `==`
  * free functions
    + `start(space)` - returns `_start`
    + `shape(space)` - returns `_shape`
    + `size(space)` - returns total number of elements
    + `contains(space, index)` - returns `bool`, checks if index is within bounds
  * iteration
    + `begin(space)` and `end(space)` - iterate over all indices in the space
    + Example: `for (auto index : space) { ... }` iterates through all valid indices
    + `for_each(space, func)` - execute `func(index)` for every index in the space
    + `for_each(space, func, exec_mode)` where `exec_mode` is one of `exec::cpu`, `exec::omp`, `exec::gpu`
  * reductions
    + `map_reduce(space, init, map, reduce_op)` - map-reduce over all indices
    + `map_reduce(space, init, map, reduce_op, exec_mode)` - with specified execution policy
    + `map` signature: `(ivec_t<S> index) -> T` - transforms index to value
    + `reduce_op` signature: `(T, T) -> T` - binary associative reduction operator
    + Example: `auto sum = map_reduce(space, 0, [&buf](auto idx) { return ndread(buf, space, idx); }, std::plus<>{});`
    + Example: `auto max = map_reduce(space, -INF, [&buf](auto idx) { return ndread(buf, space, idx); }, [](auto a, auto b) { return std::max(a, b); });`

## Multi-dimensional indexing functions
All indices are **absolute** (relative to the origin `[0, 0, ...]`). For example, with `_start = [5, 10]` and `_shape = [10, 20]`, valid indices range from `[5, 10]` to `[14, 29]` (i.e., `_start` to `_start + _shape - 1`).

- `ndoffset(space, index)` - Compute flat buffer offset from multi-dimensional index
  * `space: index_space_t<S>` - The index space defining the array dimensions
  * `index: ivec_t<S>` - Absolute multi-dimensional index
  * Returns `std::size_t` - Flat offset into buffer (row-major ordering)
  * Example: `ndoffset(space, ivec(7, 13))` with `start = [5, 10]` and `shape = [10, 20]` returns `(7-5) * 20 + (13-10) = 43`

- `ndindex(space, offset)` - Convert flat buffer offset to multi-dimensional index
  * `space: index_space_t<S>` - The index space defining the array dimensions
  * `offset: std::size_t` - Flat offset into buffer
  * Returns `ivec_t<S>` - Absolute multi-dimensional index
  * Inverse of `ndoffset`: `ndindex(space, ndoffset(space, index)) == index`

- `ndread(data, space, index)` - Read element from buffer using multi-dimensional index
  * `data: const T*` - Pointer to flat buffer
  * `space: index_space_t<S>` - Index space defining dimensions
  * `index: ivec_t<S>` - Multi-dimensional index
  * Returns `T` - Value at that index
  * Example: `ndread(buffer, space, ivec(1, 2))` reads element at row 1, column 2

- `ndwrite(data, space, index, value)` - Write element to buffer using multi-dimensional index
  * `data: T*` - Pointer to flat buffer
  * `space: index_space_t<S>` - Index space defining dimensions
  * `index: ivec_t<S>` - Multi-dimensional index
  * `value: T` - Value to write
  * Example: `ndwrite(buffer, space, ivec(1, 2), 42.0)` writes to row 1, column 2

- `ndread_soa<T, N>(data, space, index)` - Read a `vec_t<T, N>` from SoA buffer
  * Template parameters: `T` (element type), `N` (vector size)
  * `data: const T*` - Pointer to flat buffer with all components stored contiguously
  * `space: index_space_t<S>` - Index space defining spatial dimensions
  * `index: ivec_t<S>` - Multi-dimensional spatial index
  * Returns `vec_t<T, N>` - Vector with components gathered from memory
  * Layout: Component `i` of vector at `index` is at offset `i * size(space) + ndoffset(space, index)`
  * Component-major ordering: For a 2D grid `[10, 20]` storing 3-vectors (200 total positions):
    - All x-components: `[x₀, x₁, x₂, ..., x₁₉₉]`
    - All y-components: `[y₀, y₁, y₂, ..., y₁₉₉]`
    - All z-components: `[z₀, z₁, z₂, ..., z₁₉₉]`
  * Usage: `auto v = ndread_soa<double, 3>(buffer, space, ivec(1, 2));`

- `ndwrite_soa<T, N>(data, space, index, value)` - Write a `vec_t<T, N>` to SoA buffer
  * Template parameters: `T` (element type), `N` (vector size)
  * `data: T*` - Pointer to flat buffer
  * `space: index_space_t<S>` - Index space defining spatial dimensions
  * `index: ivec_t<S>` - Multi-dimensional spatial index
  * `value: vec_t<T, N>` - Vector to write
  * Scatters vector components into memory with same layout as `ndread_soa`
  * Usage: `ndwrite_soa<double, 3>(buffer, space, ivec(1, 2), dvec(1.0, 2.0, 3.0));`

# Driver

The `mist-driver.hpp` provides a generic time-stepping driver for physics simulations. It manages the main loop, adaptive time-stepping, and scheduled outputs.

## Physics Concept

Physics modules must satisfy the `Physics` concept by providing:

**Required types:**
- `config_t` - Runtime configuration (grid size, physical parameters, etc.)
- `state_t` - Conservative state variables (density, momentum, energy, etc.)
- `product_t` - Derived diagnostic quantities (velocity, pressure, etc.)

**Required functions:**
- `initial_state(config_t) -> state_t` - Generate initial conditions
- `euler_step(config_t, state_t, dt) -> state_t` - Single forward Euler step
- `courant_time(config_t, state_t) -> double` - Maximum stable timestep
- `average(state_t, state_t, alpha) -> state_t` - Convex combination: `(1-alpha)*s0 + alpha*s1`
- `get_product(config_t, state_t) -> product_t` - Compute derived quantities
- `get_time(state_t, kind) -> double` - Extract time from state
  - `kind=0`: Raw simulation time (same units as `courant_time`)
  - `kind=1, 2, ...`: Additional time variables (e.g., orbital phase, forward shock radius)
  - Physics modules define one or more time kinds based on their needs
  - **All time kinds must increase monotonically** during simulation
  - **Must throw `std::out_of_range`** if `kind` is not valid for this physics module
- `zone_count(state_t) -> std::size_t` - Number of computational zones (for performance metrics)
- `visit(func, state_t)` - Apply visitor function to all state fields
- `visit(func, product_t)` - Apply visitor function to all product fields
- `timeseries_sample(config_t, state_t) -> std::vector<std::pair<std::string, double>>` - Compute timeseries measurements
  - Returns a vector of (column_name, value) pairs
  - Column names define the available timeseries measurements
  - Order is preserved as defined by physics module
  - Example: `{{"total_mass", 1.0}, {"total_energy", 2.5}, {"max_density", 10.3}}`

## Time Integrators

The driver provides Strong Stability Preserving (SSP) Runge-Kutta methods:
- `rk1_step` - Forward Euler (1st order)
- `rk2_step` - SSP-RK2 (2nd order)
- `rk3_step` - SSP-RK3 (3rd order)

Selected via `driver_config::rk_order` (1, 2, or 3).

## Driver State

For restarts to work correctly, the driver maintains internal state that must be persisted alongside the physics state in checkpoint files.

**Driver State (must be saved in checkpoints for run continuity):**
- `int iteration` - Current iteration number
- `int message_count` - Number of iteration messages emitted
- `int checkpoint_count` - Number of checkpoints written
- `int products_count` - Number of product files written
- `int timeseries_count` - Number of timeseries samples taken
- `double next_message_time` - Next scheduled message time
- `double next_checkpoint_time` - Next scheduled checkpoint time
- `double next_products_time` - Next scheduled product output time
- `double next_timeseries_time` - Next scheduled timeseries sample time
- `std::vector<std::pair<std::string, std::vector<double>>> timeseries_data` - All timeseries data accumulated during the run
  - Structure: vector of (column_name, values) pairs
  - Each column has a name (string) and all samples for that column (vector<double>)
  - Column names and order are defined by the physics module's `timeseries_sample()` function
  - When a new sample is taken, one value is appended to each column's vector
  - Persisted across sessions so timeseries data accumulates throughout the entire run

**Session State (not persisted, lifetime of executable):**
- `double last_message_wall_time` - Wall-clock time of last message (for Mzps calculation between messages)

**Checkpoint Structure:**
When writing checkpoints, the callback receives:
- Physics `state_t` - from the physics module
- Driver state - from the driver (already updated to reflect this output)

The driver state written to the checkpoint reflects the state **after** the output has occurred:
- `checkpoint_count` has been incremented (this checkpoint's number)
- `next_checkpoint_time` has been advanced (when the next checkpoint will occur)

This ensures that restarting from a checkpoint continues correctly without re-executing the same output.

**Terminology:**
- **Run**: A simulation that may have been stopped and restarted any number of times
- **Session**: A single execution of the program (lifetime of executable)
- A run may consist of multiple sessions via checkpointing and restart

**Scheduled Output Execution Order:**
All scheduled outputs follow this uniform sequence:
1. Trigger condition is met (exact or nearest policy)
2. Increment output counter (e.g., `checkpoint_count++`)
3. Advance next scheduled time (e.g., `next_checkpoint_time += interval`)
4. Invoke checkpoint writer with state and updated driver_state

This ensures checkpoint writer always see the "post-output" driver state, which is what gets persisted.

**Restart Behavior:**
When restarting from a checkpoint:
- Driver state is restored from the checkpoint file
- Output counters continue from saved values (e.g., next checkpoint is `chkpt.0005.h5` if `checkpoint_count = 4`)
- Scheduled output times continue from saved values
- Iteration count continues from saved value
- Session state is initialized fresh (e.g., wall-clock timing resets for new session)

## Scheduled Outputs

The driver manages four types of scheduled output. Each output type can be scheduled based on any time kind defined by the physics module (via `get_time(state, kind)`), allowing outputs to recur at (semi-)regular intervals of different time variables.

**Time Kind Conventions:**
- `kind=0`: Raw simulation time (same units as `courant_time` returns)
- `kind=1+`: Physics-specific time variables (e.g. orbital phase, forward shock radius)

**Scheduling Policies:**

Each scheduled output has a policy determining how the driver hits output times:

- **Nearest**: Output occurs at the nearest iteration after the scheduled time
  - Driver checks after each step: if `time >= next_output`, trigger output and advance schedule
  - Output times may drift slightly past exact multiples of `dt_output`
  - Lower overhead, doesn't constrain timestep
  - Use for: checkpoints, diagnostics where exact timing isn't critical

- **Exact**: Driver generates output at precisely the scheduled time
  - When `time < next_output < time + dt`, creates a throw-away state advanced exactly to `next_output`
  - The throw-away state is used only for output; simulation continues from the original state
  - Guarantees outputs at exact multiples: `t = 0, dt_output, 2*dt_output, ...`
  - **Requires `time_kind = 0`** (raw simulation time) - cannot use exact scheduling with other time kinds
  - Higher overhead (extra RK step per output), but ensures exact timing
  - Use for: products, timeseries where exact timing is needed for analysis

Each scheduled output specifies an interval, a time kind, and a scheduling policy.

### 1. Iteration Messages (Console/Logging)
**Purpose:** Lightweight progress monitoring with performance metrics  
**Trigger:** Any time kind  
**Content:** Compact status (iteration, times, timestep, performance)  
**Scheduling:**
- `double message_interval` - Message interval
- `int message_interval_kind` - Time kind to use (default: 0)
- `std::string message_scheduling` - Scheduling policy: "nearest" or "exact" (default: "nearest")
  - If `"exact"`: requires `message_interval_kind = 0`

**Performance Measurement:**
- Driver measures **wall-clock time** between iteration messages
- Reports performance in **Mzps** (million zone-updates per second)
- Calculation: `Mzps = (iterations × zone_count(state)) / (wall_seconds × 1e6)`
- Requires `zone_count(state)` from physics interface
- Messages are written **after** timestep, so first message includes valid Mzps

**Message Format:**
```
[001234] t=3.14159 (1:0.5000 2:0.1233) Mzps=170.123
```
- `[001234]` - 6-digit zero-padded iteration number
- `t=3.14159` - Raw simulation time (kind=0) with 5 decimal places
- `(1:0.5000 2:0.1233)` - Additional time kinds with 4 decimal places
- `Mzps=170.123` - Performance metric with 3 decimal places

**Output Function:** Driver calls `write_iteration_message(message_string)`
- Default implementation prints to stdout
- User can override by defining their own implementation

### 2. Checkpoints (State Persistence)
**Purpose:** Save full simulation state for restart/recovery  
**Trigger:** Any time kind  
**Content:** Complete `state_t` (all conservative variables)  
**Scheduling:**
- `double checkpoint_interval` - Checkpoint interval
- `int checkpoint_interval_kind` - Time kind to use (default: 0)
- `std::string checkpoint_scheduling` - Scheduling policy: "nearest" or "exact" (default: "nearest")
  - If `"exact"`: requires `checkpoint_interval_kind = 0`

**Output Function:** Driver calls `write_checkpoint<P>(output_num, state, driver_state)`
- User must implement this template function for their physics type `P`
- Must write both `state` and `driver_state` to enable restarts
- The `driver_state` parameter contains all internal driver state needed for restart (iteration counts, next output times, accumulated timeseries data)

**Output numbering:** 
- Initial: `chkpt.0000.h5` (written at simulation start)
- Next: `chkpt.0001.h5`, `chkpt.0002.h5`, etc.
- Number is checkpoint count, not iteration number

### 3. Product Files (Derived Quantities)
**Purpose:** Write derived/diagnostic quantities for analysis  
**Trigger:** Any time kind  
**Content:** `product_t` from `get_product(cfg, state)`  
**Scheduling:**
- `double products_interval` - Product output interval
- `int products_interval_kind` - Time kind to use (default: 0)
- `std::string products_scheduling` - Scheduling policy: "nearest" or "exact" (default: "exact")
  - If `"exact"`: requires `products_interval_kind = 0`

**Output Function:** Driver calls `write_products<P>(output_num, state, product)`
- User must implement this template function for their physics type `P`
- `product` is computed by the driver via `get_product(cfg.physics, state)`

**Output numbering:**
- Initial: `prods.0000.h5` (written at `t=0`)
- Next: `prods.0001.h5`, `prods.0002.h5`, etc.
- Number is product output count, not iteration number

### 4. Timeseries Data (Scalar Diagnostics)
**Purpose:** Record scalar diagnostics over time (total energy, mass, extrema, etc.)  
**Trigger:** Any time kind  
**Content:** User-defined scalar measurements (all type `double`)  
**Scheduling:**
- `double timeseries_interval` - Timeseries sampling interval
- `int timeseries_interval_kind` - Time kind to use (default: 0)
- `std::string timeseries_scheduling` - Scheduling policy: "nearest" or "exact" (default: "exact")
  - If `"exact"`: requires `timeseries_interval_kind = 0`
- Typically more frequent than products: `timeseries_interval <= products_interval`

**Data Collection:**
- Driver calls `timeseries_sample(config, state)` from physics module
- Returns `std::vector<std::pair<std::string, double>>` with (column_name, value) pairs for this sample
- For each (name, value) pair:
  - If column `name` exists in `timeseries_data`: append `value` to that column's vector
  - If column `name` is new: create new column with `value` as first entry
- Column names do not need to be consistent across samples (columns can be added dynamically)
- All measurements are accumulated in `driver_state.timeseries_data`
- Data persists across sessions (saved in checkpoints)

**Output Function:** Driver calls `write_timeseries(timeseries_data)`
- User must implement this function
- Receives the entire accumulated timeseries data: `std::vector<std::pair<std::string, std::vector<double>>>`
- Called after each new sample is added
- Typically writes/updates a single file (e.g., `timeseries.dat` or `timeseries.h5`)

**Output behavior:**
- No numbered output files (unlike products/checkpoints)
- Single timeseries data structure that grows throughout the run
- Persisted in checkpoint files along with driver state

## Timestep and Termination

The driver uses adaptive timesteps from `courant_time(cfg, state)` multiplied by the CFL factor. The simulation terminates when:
1. `t >= t_final`, or
2. `iteration >= max_iter` (if `max_iter > 0`)

The timestep is never adjusted to hit `t_final` exactly - the simulation simply stops when the termination condition is met. Exact-policy outputs must use `time_kind = 0`.

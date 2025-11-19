#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include "mist/core.hpp"
#include "mist/driver.hpp"

using namespace mist;

// =============================================================================
// 1D Linear Advection Physics Module
// =============================================================================

struct advection_1d {

    // Configuration: runtime parameters
    struct config_t {
        unsigned int num_zones;
        double domain_length;
        double advection_velocity;
    };

    // State: conservative variables + metadata
    struct state_t {
        std::vector<double> conserved;
        double time;
        index_space_t<1> grid;
    };

    // Product: derived quantities
    struct product_t {
        std::vector<double> primitive;
        double total_mass;
        double min_value;
        double max_value;
    };
};

// Initial state: sine wave
auto initial_state(const advection_1d::config_t& cfg) -> advection_1d::state_t {
    auto grid = index_space(ivec(0), uvec(cfg.num_zones));
    std::vector<double> u(cfg.num_zones);
    double dx = cfg.domain_length / cfg.num_zones;

    for (unsigned int i = 0; i < cfg.num_zones; ++i) {
        double x = (i + 0.5) * dx;
        u[i] = std::sin(2.0 * M_PI * x / cfg.domain_length);
    }

    return {u, 0.0, grid};
}

// Forward Euler step
auto euler_step(
    const advection_1d::config_t& cfg,
    const advection_1d::state_t& state,
    double dt
) -> advection_1d::state_t {

    auto new_state = state;
    new_state.time += dt;

    double dx = cfg.domain_length / cfg.num_zones;
    double v = cfg.advection_velocity;

    // First-order upwind scheme
    for (unsigned int i = 0; i < cfg.num_zones; ++i) {
        unsigned int im1 = (i == 0) ? cfg.num_zones - 1 : i - 1;

        if (v > 0) {
            double flux_left = v * state.conserved[im1];
            double flux_right = v * state.conserved[i];
            new_state.conserved[i] = state.conserved[i] - dt / dx * (flux_right - flux_left);
        } else {
            unsigned int ip1 = (i + 1) % cfg.num_zones;
            double flux_left = v * state.conserved[i];
            double flux_right = v * state.conserved[ip1];
            new_state.conserved[i] = state.conserved[i] - dt / dx * (flux_right - flux_left);
        }
    }

    return new_state;
}

// CFL timestep
auto courant_time(
    const advection_1d::config_t& cfg,
    const advection_1d::state_t& state
) -> double {
    double dx = cfg.domain_length / cfg.num_zones;
    double v = std::abs(cfg.advection_velocity);
    return dx / v;
}

// Weighted average of two states
auto average(
    const advection_1d::state_t& s1,
    const advection_1d::state_t& s2,
    double alpha
) -> advection_1d::state_t {

    auto result = s1;

    for (unsigned int i = 0; i < s1.conserved.size(); ++i) {
        result.conserved[i] = (1.0 - alpha) * s1.conserved[i] + alpha * s2.conserved[i];
    }

    result.time = (1.0 - alpha) * s1.time + alpha * s2.time;
    return result;
}

// Compute diagnostics
auto get_product(
    const advection_1d::config_t& cfg,
    const advection_1d::state_t& state
) -> advection_1d::product_t {

    double dx = cfg.domain_length / cfg.num_zones;
    double total_mass = 0.0;
    double min_val = state.conserved[0];
    double max_val = state.conserved[0];

    for (const auto& u : state.conserved) {
        total_mass += u * dx;
        min_val = std::min(min_val, u);
        max_val = std::max(max_val, u);
    }

    return {state.conserved, total_mass, min_val, max_val};
}

// Get time for scheduling
auto get_time(const advection_1d::state_t& state, int kind) -> double {
    if (kind == 0) return state.time;
    throw std::out_of_range("advection_1d only supports time kind=0");
}

// Zone count for performance metrics
auto zone_count(const advection_1d::state_t& state) -> std::size_t {
    return state.conserved.size();
}

// Timeseries samples
auto timeseries_sample(
    const advection_1d::config_t& cfg,
    const advection_1d::state_t& state
) -> std::vector<std::pair<std::string, double>> {

    auto product = get_product(cfg, state);
    return {
        {"time", state.time},
        {"total_mass", product.total_mass},
        {"min_value", product.min_value},
        {"max_value", product.max_value}
    };
}

// Visitor for State
template<typename Visitor>
void visit(Visitor&& v, advection_1d::state_t& s) {
    v("conserved", s.conserved);
    v("time", s.time);
}

// Visitor for Product
template<typename Visitor>
void visit(Visitor&& v, advection_1d::product_t& p) {
    v("primitive", p.primitive);
    v("total_mass", p.total_mass);
    v("min_value", p.min_value);
    v("max_value", p.max_value);
}

// =============================================================================
// Output function implementations
// =============================================================================

namespace mist {

template<>
void write_checkpoint<advection_1d>(
    int output_num,
    const advection_1d::state_t& state,
    const driver_state_t& driver_state)
{
    char filename[64];
    std::snprintf(filename, sizeof(filename), "chkpt.%04d.dat", output_num);
    std::ofstream file(filename);
    
    file << "# Checkpoint " << output_num << "\n";
    file << "# time = " << state.time << "\n";
    file << "# iteration = " << driver_state.iteration << "\n";
    
    for (const auto& u : state.conserved) {
        file << u << "\n";
    }
}

template<>
void write_products<advection_1d>(
    int output_num,
    const advection_1d::state_t& state,
    const advection_1d::product_t& product)
{
    char filename[64];
    std::snprintf(filename, sizeof(filename), "prods.%04d.dat", output_num);
    std::ofstream file(filename);
    
    file << "# Product " << output_num << "\n";
    file << "# time = " << state.time << "\n";
    file << "# total_mass = " << product.total_mass << "\n";
    file << "# min_value = " << product.min_value << "\n";
    file << "# max_value = " << product.max_value << "\n";
    
    for (const auto& u : product.primitive) {
        file << u << "\n";
    }
}

void write_timeseries(
    const std::vector<std::pair<std::string, std::vector<double>>>& timeseries_data)
{
    std::ofstream file("timeseries.dat");
    
    // Write header
    file << "# ";
    for (const auto& [name, values] : timeseries_data) {
        file << name << " ";
    }
    file << "\n";
    
    // Write data rows
    if (!timeseries_data.empty()) {
        std::size_t num_samples = timeseries_data[0].second.size();
        for (std::size_t i = 0; i < num_samples; ++i) {
            for (const auto& [name, values] : timeseries_data) {
                if (i < values.size()) {
                    file << values[i] << " ";
                }
            }
            file << "\n";
        }
    }
}

} // namespace mist

// =============================================================================
// Main driver
// =============================================================================

int main() {
    static_assert(Physics<advection_1d>, "advection_1d must satisfy Physics concept");

    std::cout << "=== 1D Linear Advection Demo (Mist Driver) ===\n\n";

    // Setup configuration
    driver_config<advection_1d> cfg;
    cfg.physics = {
        .num_zones = 200,
        .domain_length = 1.0,
        .advection_velocity = 1.0
    };

    // Driver settings
    cfg.rk_order = 3;
    cfg.cfl = 0.5;
    cfg.t_final = 2.0;
    cfg.max_iter = -1;

    // Output scheduling
    cfg.message_interval = 0.1;
    cfg.message_interval_kind = 0;
    cfg.message_scheduling = "nearest";

    cfg.checkpoint_interval = 0.5;
    cfg.checkpoint_interval_kind = 0;
    cfg.checkpoint_scheduling = "nearest";

    cfg.products_interval = 0.2;
    cfg.products_interval_kind = 0;
    cfg.products_scheduling = "exact";

    cfg.timeseries_interval = 0.05;
    cfg.timeseries_interval_kind = 0;
    cfg.timeseries_scheduling = "exact";

    std::cout << "Configuration:\n";
    std::cout << "  Grid zones: " << cfg.physics.num_zones << "\n";
    std::cout << "  Domain length: " << cfg.physics.domain_length << "\n";
    std::cout << "  Advection velocity: " << cfg.physics.advection_velocity << "\n";
    std::cout << "  RK order: " << cfg.rk_order << "\n";
    std::cout << "  CFL: " << cfg.cfl << "\n";
    std::cout << "  Final time: " << cfg.t_final << "\n\n";

    // Run simulation
    auto final_state = run(cfg);

    std::cout << "\n=== Simulation Complete ===\n";
    std::cout << "Final time: " << final_state.time << "\n";

    return 0;
}

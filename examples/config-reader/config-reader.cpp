#include <iostream>
#include <fstream>
#include <cstdlib>
#include "mist/core.hpp"
#include "mist/serialize.hpp"
#include "mist/ascii_reader.hpp"
#include "mist/ascii_writer.hpp"

using namespace mist;

// =============================================================================
// Nested configuration structures
// =============================================================================

struct boundary_t {
    int type;                    // 0=periodic, 1=outflow, 2=reflecting
    double value;                // boundary value (if applicable)

    auto fields() const {
        return std::make_tuple(
            field("type", type),
            field("value", value)
        );
    }

    auto fields() {
        return std::make_tuple(
            field("type", type),
            field("value", value)
        );
    }
};

struct mesh_t {
    vec_t<int, 3> resolution;
    vec_t<double, 3> lower;
    vec_t<double, 3> upper;
    boundary_t boundary_lo;
    boundary_t boundary_hi;

    auto fields() const {
        return std::make_tuple(
            field("resolution", resolution),
            field("lower", lower),
            field("upper", upper),
            field("boundary_lo", boundary_lo),
            field("boundary_hi", boundary_hi)
        );
    }

    auto fields() {
        return std::make_tuple(
            field("resolution", resolution),
            field("lower", lower),
            field("upper", upper),
            field("boundary_lo", boundary_lo),
            field("boundary_hi", boundary_hi)
        );
    }
};

struct physics_t {
    double gamma;
    double cfl;
    std::vector<double> diffusion_coeffs;

    auto fields() const {
        return std::make_tuple(
            field("gamma", gamma),
            field("cfl", cfl),
            field("diffusion_coeffs", diffusion_coeffs)
        );
    }

    auto fields() {
        return std::make_tuple(
            field("gamma", gamma),
            field("cfl", cfl),
            field("diffusion_coeffs", diffusion_coeffs)
        );
    }
};

struct source_t {
    std::string name;
    vec_t<double, 3> position;
    vec_t<double, 3> velocity;
    double radius;
    double amplitude;

    auto fields() const {
        return std::make_tuple(
            field("name", name),
            field("position", position),
            field("velocity", velocity),
            field("radius", radius),
            field("amplitude", amplitude)
        );
    }

    auto fields() {
        return std::make_tuple(
            field("name", name),
            field("position", position),
            field("velocity", velocity),
            field("radius", radius),
            field("amplitude", amplitude)
        );
    }
};

struct output_t {
    std::string directory;
    std::string prefix;
    std::vector<double> snapshot_times;
    int checkpoint_interval;
    double timeseries_dt;

    auto fields() const {
        return std::make_tuple(
            field("directory", directory),
            field("prefix", prefix),
            field("snapshot_times", snapshot_times),
            field("checkpoint_interval", checkpoint_interval),
            field("timeseries_dt", timeseries_dt)
        );
    }

    auto fields() {
        return std::make_tuple(
            field("directory", directory),
            field("prefix", prefix),
            field("snapshot_times", snapshot_times),
            field("checkpoint_interval", checkpoint_interval),
            field("timeseries_dt", timeseries_dt)
        );
    }
};

struct config_t {
    std::string title;
    std::string description;
    int version;
    double t_final;
    int max_iterations;
    mesh_t mesh;
    physics_t physics;
    std::vector<source_t> sources;
    output_t output;

    auto fields() const {
        return std::make_tuple(
            field("title", title),
            field("description", description),
            field("version", version),
            field("t_final", t_final),
            field("max_iterations", max_iterations),
            field("mesh", mesh),
            field("physics", physics),
            field("sources", sources),
            field("output", output)
        );
    }

    auto fields() {
        return std::make_tuple(
            field("title", title),
            field("description", description),
            field("version", version),
            field("t_final", t_final),
            field("max_iterations", max_iterations),
            field("mesh", mesh),
            field("physics", physics),
            field("sources", sources),
            field("output", output)
        );
    }
};

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: cannot open file '" << argv[1] << "'\n";
        return 1;
    }

    try {
        ascii_reader reader(file);
        config_t config;
        deserialize(reader, "config", config);

        std::cout << "Configuration loaded successfully!\n";
        std::cout << "========================================\n\n";

        // Output the configuration using ascii_writer
        ascii_writer writer(std::cout);
        serialize(writer, "config", config);

    } catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

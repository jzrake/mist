#include <iostream>
#include <sstream>
#include <cassert>
#include <cmath>
#include "mist/core.hpp"
#include "mist/serialize.hpp"
#include "mist/ascii_writer.hpp"
#include "mist/ascii_reader.hpp"

using namespace mist;

// =============================================================================
// Test structures (matching the README specification)
// =============================================================================

struct particle_t {
    vec_t<double, 3> position;
    vec_t<double, 3> velocity;
    double mass;

    auto fields() const {
        return std::make_tuple(
            field("position", position),
            field("velocity", velocity),
            field("mass", mass)
        );
    }

    auto fields() {
        return std::make_tuple(
            field("position", position),
            field("velocity", velocity),
            field("mass", mass)
        );
    }
};

struct grid_config_t {
    vec_t<int, 3> resolution;
    vec_t<double, 3> domain_min;
    vec_t<double, 3> domain_max;

    auto fields() const {
        return std::make_tuple(
            field("resolution", resolution),
            field("domain_min", domain_min),
            field("domain_max", domain_max)
        );
    }

    auto fields() {
        return std::make_tuple(
            field("resolution", resolution),
            field("domain_min", domain_min),
            field("domain_max", domain_max)
        );
    }
};

struct simulation_state_t {
    double time;
    int iteration;
    grid_config_t grid;
    std::vector<particle_t> particles;
    std::vector<double> scalar_field;

    auto fields() const {
        return std::make_tuple(
            field("time", time),
            field("iteration", iteration),
            field("grid", grid),
            field("particles", particles),
            field("scalar_field", scalar_field)
        );
    }

    auto fields() {
        return std::make_tuple(
            field("time", time),
            field("iteration", iteration),
            field("grid", grid),
            field("particles", particles),
            field("scalar_field", scalar_field)
        );
    }
};

// =============================================================================
// Helper functions
// =============================================================================

bool approx_equal(double a, double b, double tol = 1e-10) {
    return std::abs(a - b) < tol;
}

template<typename T, std::size_t N>
bool vec_equal(const vec_t<T, N>& a, const vec_t<T, N>& b) {
    for (std::size_t i = 0; i < N; ++i) {
        if constexpr (std::is_floating_point_v<T>) {
            if (!approx_equal(a[i], b[i])) return false;
        } else {
            if (a[i] != b[i]) return false;
        }
    }
    return true;
}

// =============================================================================
// Tests
// =============================================================================

void test_scalar_serialization() {
    std::cout << "Testing scalar serialization... ";

    std::stringstream ss;
    ascii_writer writer(ss);

    writer.write_scalar("time", 1.234);
    writer.write_scalar("iteration", 42);

    std::string output = ss.str();
    assert(output.find("time = ") != std::string::npos);
    assert(output.find("iteration = 42") != std::string::npos);

    // Test round-trip
    ss.seekg(0);
    ascii_reader reader(ss);

    double time;
    int iteration;
    reader.read_scalar("time", time);
    reader.read_scalar("iteration", iteration);

    assert(approx_equal(time, 1.234));
    assert(iteration == 42);

    std::cout << "PASSED\n";
}

void test_vec_serialization() {
    std::cout << "Testing vec_t serialization... ";

    vec_t<double, 3> original = {1.5, 2.5, 3.5};

    std::stringstream ss;
    ascii_writer writer(ss);
    writer.write_array("position", original);

    std::string output = ss.str();
    assert(output.find("position = [") != std::string::npos);

    ss.seekg(0);
    ascii_reader reader(ss);

    vec_t<double, 3> loaded = {};
    reader.read_array("position", loaded);

    assert(vec_equal(original, loaded));

    std::cout << "PASSED\n";
}

void test_scalar_vector_serialization() {
    std::cout << "Testing std::vector<double> serialization... ";

    std::vector<double> original = {300.0, 305.2, 298.5, 302.1};

    std::stringstream ss;
    ascii_writer writer(ss);
    writer.write_array("scalar_field", original);

    std::string output = ss.str();
    assert(output.find("scalar_field = [") != std::string::npos);

    ss.seekg(0);
    ascii_reader reader(ss);

    std::vector<double> loaded;
    reader.read_array("scalar_field", loaded);

    assert(original.size() == loaded.size());
    for (std::size_t i = 0; i < original.size(); ++i) {
        assert(approx_equal(original[i], loaded[i]));
    }

    std::cout << "PASSED\n";
}

void test_nested_struct_serialization() {
    std::cout << "Testing nested struct serialization... ";

    grid_config_t original;
    original.resolution = {64, 64, 32};
    original.domain_min = {0.0, 0.0, 0.0};
    original.domain_max = {1.0, 1.0, 0.5};

    std::stringstream ss;
    ascii_writer writer(ss);
    serialize(writer, "grid", original);

    std::string output = ss.str();
    assert(output.find("grid {") != std::string::npos);
    assert(output.find("resolution = [64, 64, 32]") != std::string::npos);

    ss.seekg(0);
    ascii_reader reader(ss);

    grid_config_t loaded;
    deserialize(reader, "grid", loaded);

    assert(vec_equal(original.resolution, loaded.resolution));
    assert(vec_equal(original.domain_min, loaded.domain_min));
    assert(vec_equal(original.domain_max, loaded.domain_max));

    std::cout << "PASSED\n";
}

void test_compound_vector_serialization() {
    std::cout << "Testing std::vector<compound> serialization... ";

    std::vector<particle_t> original = {
        {{0.1, 0.2, 0.15}, {1.5, -0.3, 0.0}, 1.0},
        {{0.8, 0.7, 0.25}, {-0.5, 0.8, 0.2}, 2.0}
    };

    std::stringstream ss;
    ascii_writer writer(ss);
    serialize(writer, "particles", original);

    std::string output = ss.str();
    assert(output.find("particles {") != std::string::npos);

    ss.seekg(0);
    ascii_reader reader(ss);

    std::vector<particle_t> loaded;
    deserialize(reader, "particles", loaded);

    assert(original.size() == loaded.size());
    for (std::size_t i = 0; i < original.size(); ++i) {
        assert(vec_equal(original[i].position, loaded[i].position));
        assert(vec_equal(original[i].velocity, loaded[i].velocity));
        assert(approx_equal(original[i].mass, loaded[i].mass));
    }

    std::cout << "PASSED\n";
}

void test_full_simulation_state() {
    std::cout << "Testing full simulation_state_t serialization... ";

    simulation_state_t original;
    original.time = 1.234;
    original.iteration = 42;
    original.grid.resolution = {64, 64, 32};
    original.grid.domain_min = {0.0, 0.0, 0.0};
    original.grid.domain_max = {1.0, 1.0, 0.5};
    original.particles = {
        {{0.1, 0.2, 0.15}, {1.5, -0.3, 0.0}, 1.2},
        {{0.8, 0.7, 0.25}, {-0.5, 0.8, 0.2}, 1.1}
    };
    original.scalar_field = {300.0, 305.2, 298.5, 302.1};

    std::stringstream ss;
    ascii_writer writer(ss);
    serialize(writer, "simulation_state", original);

    std::cout << "\n--- Serialized Output ---\n";
    std::cout << ss.str();
    std::cout << "--- End Output ---\n";

    ss.seekg(0);
    ascii_reader reader(ss);

    simulation_state_t loaded;
    deserialize(reader, "simulation_state", loaded);

    assert(approx_equal(original.time, loaded.time));
    assert(original.iteration == loaded.iteration);
    assert(vec_equal(original.grid.resolution, loaded.grid.resolution));
    assert(vec_equal(original.grid.domain_min, loaded.grid.domain_min));
    assert(vec_equal(original.grid.domain_max, loaded.grid.domain_max));
    assert(original.particles.size() == loaded.particles.size());
    assert(original.scalar_field.size() == loaded.scalar_field.size());

    std::cout << "PASSED\n";
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== ASCII Serialization Tests ===\n\n";

    test_scalar_serialization();
    test_vec_serialization();
    test_scalar_vector_serialization();
    test_nested_struct_serialization();
    test_compound_vector_serialization();
    test_full_simulation_state();

    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}

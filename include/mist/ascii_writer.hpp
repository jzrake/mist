#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <iomanip>
#include "core.hpp"

namespace mist {

// =============================================================================
// ASCII Writer
// =============================================================================

class ascii_writer {
public:
    explicit ascii_writer(std::ostream& os, int indent_size = 4)
        : os_(os), indent_size_(indent_size), indent_level_(0), need_newline_(false) {}

    // =========================================================================
    // Scalar types
    // =========================================================================

    template<typename T>
        requires std::is_arithmetic_v<T>
    void write_scalar(const char* name, const T& value) {
        write_indent();
        os_ << name << " = " << format_value(value) << "\n";
    }

    // =========================================================================
    // String type
    // =========================================================================

    void write_string(const char* name, const std::string& value) {
        write_indent();
        os_ << name << " = \"" << escape_string(value) << "\"\n";
    }

    // =========================================================================
    // vec_t<T, N> - inline comma-separated arrays
    // =========================================================================

    template<typename T, std::size_t N>
    void write_vec(const char* name, const vec_t<T, N>& value) {
        write_indent();
        os_ << name << " = [";
        for (std::size_t i = 0; i < N; ++i) {
            if (i > 0) os_ << ", ";
            os_ << format_value(value[i]);
        }
        os_ << "]\n";
    }

    // =========================================================================
    // std::vector<T> where T is arithmetic - inline comma-separated arrays
    // =========================================================================

    template<typename T>
        requires std::is_arithmetic_v<T>
    void write_scalar_vector(const char* name, const std::vector<T>& value) {
        write_indent();
        os_ << name << " = [";
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (i > 0) os_ << ", ";
            os_ << format_value(value[i]);
        }
        os_ << "]\n";
    }

    // =========================================================================
    // Compound vector support
    // =========================================================================

    void begin_compound_vector(const char* name, [[maybe_unused]] std::size_t count) {
        write_indent();
        os_ << name << " {\n";
        indent_level_++;
    }

    void end_compound_vector() {
        indent_level_--;
        write_indent();
        os_ << "}\n";
    }

    void begin_compound_vector_element([[maybe_unused]] std::size_t index) {
        write_indent();
        os_ << "{\n";
        indent_level_++;
    }

    void end_compound_vector_element() {
        indent_level_--;
        write_indent();
        os_ << "}\n";
    }

    // =========================================================================
    // Nested structure support
    // =========================================================================

    void begin_group(const char* name) {
        write_indent();
        os_ << name << " {\n";
        indent_level_++;
    }

    void end_group() {
        indent_level_--;
        write_indent();
        os_ << "}\n";
    }

private:
    std::ostream& os_;
    int indent_size_;
    int indent_level_;
    bool need_newline_;

    void write_indent() {
        for (int i = 0; i < indent_level_ * indent_size_; ++i) {
            os_ << ' ';
        }
    }

    template<typename T>
    static std::string format_value(const T& value) {
        if constexpr (std::is_floating_point_v<T>) {
            std::ostringstream oss;
            oss << std::setprecision(15) << value;
            std::string s = oss.str();
            // Ensure floating point values have a decimal point
            if (s.find('.') == std::string::npos && s.find('e') == std::string::npos) {
                s += ".0";
            }
            return s;
        } else {
            return std::to_string(value);
        }
    }

    static std::string escape_string(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '\\': result += "\\\\"; break;
                case '"':  result += "\\\""; break;
                case '\n': result += "\\n"; break;
                case '\t': result += "\\t"; break;
                case '\r': result += "\\r"; break;
                default:   result += c; break;
            }
        }
        return result;
    }
};

} // namespace mist

#pragma once

#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cctype>
#include "core.hpp"

namespace mist {

// =============================================================================
// ASCII Reader
// =============================================================================

class ascii_reader {
public:
    explicit ascii_reader(std::istream& is)
        : is_(is), current_group_("") {}

    // =========================================================================
    // Scalar types
    // =========================================================================

    template<typename T>
        requires std::is_arithmetic_v<T>
    void read_scalar(const char* name, T& value) {
        skip_whitespace_and_comments();
        std::string field_name = read_identifier();
        if (field_name != name) {
            throw std::runtime_error(
                "Expected field '" + std::string(name) + "' but found '" + field_name + 
                "' in group '" + current_group_ + "'");
        }
        skip_whitespace();
        expect_char('=');
        skip_whitespace();
        value = read_value<T>();
    }

    // =========================================================================
    // String type
    // =========================================================================

    void read_string(const char* name, std::string& value) {
        skip_whitespace_and_comments();
        std::string field_name = read_identifier();
        if (field_name != name) {
            throw std::runtime_error(
                "Expected field '" + std::string(name) + "' but found '" + field_name + 
                "' in group '" + current_group_ + "'");
        }
        skip_whitespace();
        expect_char('=');
        skip_whitespace();
        value = read_quoted_string();
    }

    // =========================================================================
    // vec_t<T, N>
    // =========================================================================

    template<typename T, std::size_t N>
    void read_vec(const char* name, vec_t<T, N>& value) {
        skip_whitespace_and_comments();
        std::string field_name = read_identifier();
        if (field_name != name) {
            throw std::runtime_error(
                "Expected field '" + std::string(name) + "' but found '" + field_name + 
                "' in group '" + current_group_ + "'");
        }
        skip_whitespace();
        expect_char('=');
        skip_whitespace();
        expect_char('[');
        for (std::size_t i = 0; i < N; ++i) {
            skip_whitespace();
            value[i] = read_value<T>();
            skip_whitespace();
            if (i < N - 1) {
                expect_char(',');
            }
        }
        skip_whitespace();
        expect_char(']');
    }

    // =========================================================================
    // std::vector<T> where T is arithmetic
    // =========================================================================

    template<typename T>
        requires std::is_arithmetic_v<T>
    void read_scalar_vector(const char* name, std::vector<T>& value) {
        skip_whitespace_and_comments();
        std::string field_name = read_identifier();
        if (field_name != name) {
            throw std::runtime_error(
                "Expected field '" + std::string(name) + "' but found '" + field_name + 
                "' in group '" + current_group_ + "'");
        }
        skip_whitespace();
        expect_char('=');
        skip_whitespace();
        expect_char('[');
        
        value.clear();
        skip_whitespace();
        
        // Handle empty vector
        if (peek_char() == ']') {
            get_char();
            return;
        }
        
        while (true) {
            skip_whitespace();
            value.push_back(read_value<T>());
            skip_whitespace();
            char c = peek_char();
            if (c == ',') {
                get_char();
            } else if (c == ']') {
                get_char();
                break;
            } else {
                throw std::runtime_error(
                    "Expected ',' or ']' but found '" + std::string(1, c) + "'");
            }
        }
    }

    // =========================================================================
    // Compound vector support
    // =========================================================================

    std::size_t begin_compound_vector(const char* name) {
        skip_whitespace_and_comments();
        std::string field_name = read_identifier();
        if (field_name != name) {
            throw std::runtime_error(
                "Expected field '" + std::string(name) + "' but found '" + field_name + 
                "' in group '" + current_group_ + "'");
        }
        skip_whitespace();
        expect_char('{');
        
        // Count elements by scanning ahead
        std::streampos start_pos = is_.tellg();
        std::size_t count = count_compound_elements();
        is_.seekg(start_pos);
        
        std::string prev_group = current_group_;
        current_group_ = current_group_.empty() ? name : current_group_ + "/" + name;
        group_stack_.push_back(prev_group);
        
        return count;
    }

    void end_compound_vector() {
        skip_whitespace_and_comments();
        expect_char('}');
        if (!group_stack_.empty()) {
            current_group_ = group_stack_.back();
            group_stack_.pop_back();
        }
    }

    void begin_compound_vector_element([[maybe_unused]] std::size_t index) {
        skip_whitespace_and_comments();
        expect_char('{');
    }

    void end_compound_vector_element() {
        skip_whitespace_and_comments();
        expect_char('}');
    }

    // =========================================================================
    // Nested structure support
    // =========================================================================

    void begin_group(const char* name) {
        skip_whitespace_and_comments();
        std::string field_name = read_identifier();
        if (field_name != name) {
            throw std::runtime_error(
                "Expected group '" + std::string(name) + "' but found '" + field_name + 
                "' in group '" + current_group_ + "'");
        }
        skip_whitespace();
        expect_char('{');
        
        std::string prev_group = current_group_;
        current_group_ = current_group_.empty() ? name : current_group_ + "/" + name;
        group_stack_.push_back(prev_group);
    }

    void end_group() {
        skip_whitespace_and_comments();
        expect_char('}');
        if (!group_stack_.empty()) {
            current_group_ = group_stack_.back();
            group_stack_.pop_back();
        }
    }

private:
    std::istream& is_;
    std::string current_group_;
    std::vector<std::string> group_stack_;

    char peek_char() {
        return static_cast<char>(is_.peek());
    }

    char get_char() {
        return static_cast<char>(is_.get());
    }

    void skip_whitespace() {
        while (is_ && std::isspace(peek_char())) {
            get_char();
        }
    }

    void skip_whitespace_and_comments() {
        while (is_) {
            skip_whitespace();
            if (peek_char() == '#') {
                // Skip comment line
                while (is_ && get_char() != '\n') {}
            } else {
                break;
            }
        }
    }

    void expect_char(char expected) {
        char c = get_char();
        if (c != expected) {
            throw std::runtime_error(
                "Expected '" + std::string(1, expected) + "' but found '" + 
                std::string(1, c) + "' in group '" + current_group_ + "'");
        }
    }

    std::string read_identifier() {
        std::string result;
        while (is_) {
            char c = peek_char();
            if (std::isalnum(c) || c == '_') {
                result += get_char();
            } else {
                break;
            }
        }
        if (result.empty()) {
            throw std::runtime_error("Expected identifier in group '" + current_group_ + "'");
        }
        return result;
    }

    template<typename T>
    T read_value() {
        std::string token;
        while (is_) {
            char c = peek_char();
            if (std::isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') {
                token += get_char();
            } else {
                break;
            }
        }
        
        if (token.empty()) {
            throw std::runtime_error("Expected numeric value in group '" + current_group_ + "'");
        }
        
        T value;
        std::istringstream iss(token);
        iss >> value;
        if (iss.fail()) {
            throw std::runtime_error("Failed to parse value '" + token + "' in group '" + current_group_ + "'");
        }
        return value;
    }

    std::string read_quoted_string() {
        expect_char('"');
        std::string result;
        while (is_) {
            char c = get_char();
            if (c == '"') {
                break;
            } else if (c == '\\') {
                char next = get_char();
                switch (next) {
                    case '\\': result += '\\'; break;
                    case '"':  result += '"'; break;
                    case 'n':  result += '\n'; break;
                    case 't':  result += '\t'; break;
                    case 'r':  result += '\r'; break;
                    default:   result += next; break;
                }
            } else {
                result += c;
            }
        }
        return result;
    }

    std::size_t count_compound_elements() {
        // Count brace pairs at the current nesting level
        std::size_t count = 0;
        int depth = 0;
        bool in_element = false;
        
        while (is_) {
            skip_whitespace_and_comments();
            char c = peek_char();
            
            if (c == '{') {
                get_char();
                if (depth == 0) {
                    in_element = true;
                }
                depth++;
            } else if (c == '}') {
                if (depth == 0) {
                    // End of compound vector
                    break;
                }
                get_char();
                depth--;
                if (depth == 0 && in_element) {
                    count++;
                    in_element = false;
                }
            } else if (c == std::char_traits<char>::eof()) {
                break;
            } else {
                get_char();
            }
        }
        
        return count;
    }
};

} // namespace mist

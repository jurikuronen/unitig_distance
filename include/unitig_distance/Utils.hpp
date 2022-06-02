#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "types.hpp"

class Utils {
public:
    static std::vector<std::string> get_fields(const std::string& line, char delim = ' ') {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        for (std::string field; std::getline(ss, field, delim); ) fields.push_back(std::move(field));
        return fields;
    }

    static bool file_is_good(const std::string& filename) {
        return std::ifstream(filename).good();
    }

    static std::string neat_number_str(int_t number) {
        std::vector<int_t> parts;
        do parts.push_back(number % 1000);
        while (number /= 1000);
        std::string number_str = std::to_string(parts.back());
        for (int_t i = parts.size() - 2; i >= 0; --i) {
            number_str += ' ' + std::string(3 - std::to_string(parts[i]).size(), '0') + std::to_string(parts[i]);
        }
        return number_str;
    }

    static std::string neat_decimal_str(int_t nom, int_t denom) {
        std::string int_str = std::to_string(nom / denom);
        std::string dec_str = std::to_string(nom * 100 / denom % 100);
        return int_str + "." + std::string(2 - dec_str.size(), '0') + dec_str;
    }

    static real_t fixed_distance(real_t distance, real_t max_distance = REAL_T_MAX) { return distance >= max_distance ? -1.0 : distance; }

    static int_t left_node(int_t v) { return v * 2; }
    static int_t right_node(int_t v) { return v * 2 + 1; }

    static bool is_numeric(const std::string& str) {
        double x;
        return (std::stringstream(str) >> x).eof();
    }

    template <typename T>
    static void clear(T& container) { T().swap(container); }

private:

};

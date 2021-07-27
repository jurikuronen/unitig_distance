/*
    Define types used by the program.
*/

#pragma once

#include <cstdint>
#include <limits>
#include <string>

using int_t = int64_t;
const int_t INT_T_MAX = std::numeric_limits<int_t>::max();

// Put this here temporarily.
inline std::string neat_number_str(int_t number) {
    std::vector<int_t> parts;
    do {
        parts.push_back(number % 1000);
    } while (number /= 1000);
    std::string number_str = std::to_string(parts.back());
    for (int_t i = parts.size() - 2; i >= 0; --i) {
        number_str += ' ' + std::string(3 - std::to_string(parts[i]).size(), '0') + std::to_string(parts[i]);
    }
    return number_str;
}


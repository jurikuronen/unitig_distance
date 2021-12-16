#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "types.hpp"

namespace unitig_distance {

    std::vector<std::string> get_fields(const std::string& line, char delim = ' ');
    bool file_is_good(const std::string& filename);

    std::string neat_number_str(int_t number);

    int_t fixed_distance(real_t distance, real_t max_distance);

    int_t left_node(int_t v);
    int_t right_node(int_t v);

    template <typename T, int IDX>
    std::vector<T> transform_distance_tuple_vector(const std::vector<std::tuple<real_t, real_t, real_t, int_t>>& tuple_vector);

}

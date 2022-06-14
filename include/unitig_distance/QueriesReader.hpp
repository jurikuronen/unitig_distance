#pragma once

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "DistanceVector.hpp"
#include "Queries.hpp"
#include "Utils.hpp"

/*
    Class which handles reading queries. Supports process substitution input.
    Queries file is allowed to follow one of the following formats:
     0: v w
     1: v w score
     2: v w distance score
     3: v w distance flag score
     4: v w distance score count
     5: v w distance flag score count
*/
class QueriesReader {
public:
    static Queries read_queries() {
        std::ifstream ifs(ProgramOptions::queries_filename);
        std::string line;
        std::getline(ifs, line);

        int_t queries_format = ProgramOptions::queries_format < 0 ? Utils::deduce_queries_format(line) : ProgramOptions::queries_format;
        if (queries_format < 0) {
            std::cerr << "Could not automatically deduce queries format. Please set it with option -q [ --queries-type ] arg." << std::endl;
            return Queries();
        }
        if (ProgramOptions::operating_mode == OperatingMode::OUTLIER_TOOLS && queries_format < 4) {
            std::cerr << "Not enough columns (5 or 6 required) in queries file for outlier tools mode." << std::endl;
            return Queries();
        }
        std::cout << "Reading queries with format: " << Utils::get_queries_format_string(queries_format) << std::endl;

        Queries queries(queries_format);

        int_t distance_field, flag_field, score_field, count_field;
        std::tie(distance_field, flag_field, score_field, count_field) = Utils::get_field_indices(queries_format);
        std::size_t n_fields = queries_format + 2 - (queries_format > 3);

        if (distance_field && count_field) queries.set_mean_distances();

        int_t n = 0;

        do {
            auto fields = Utils::get_fields(line);
            if (fields.size() < n_fields) {
                print_error(line, n_fields, n + 1);
                return Queries();
            }
            int_t v = std::stoll(fields[0]) - ProgramOptions::queries_one_based;
            int_t w = std::stoll(fields[1]) - ProgramOptions::queries_one_based;
            queries.add_vertices(v, w);
            if (flag_field) queries.add_flag(std::stoi(fields[flag_field]));
            if (score_field) queries.add_score(std::stod(fields[score_field]));
            if (distance_field) {
                real_t distance = std::stod(fields[distance_field]);
                int_t count = count_field ? std::stoll(fields[count_field]) : 1;
                queries.add_distance(distance, count);
            }

            if (++n >= ProgramOptions::n_queries) break;
        } while (std::getline(ifs, line));

        return queries;
    }

private:
    static void print_error(const std::string& line, int_t n_columns, int_t count) {
        std::cerr << "Not enough columns (" << n_columns << " required) in queries file \"" << ProgramOptions::queries_filename
                  << "\" line " << count << " \"" << line << "\". Is the file space-separated?" << std::endl;
    }

};

#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "DistanceVector.hpp"
#include "Queries.hpp"
#include "Utils.hpp"

using po = ProgramOptions;

class QueriesReader {
public:
    static Queries read_queries() {
        if (ProgramOptions::queries_type == 0) return read_general_input();
        if (ProgramOptions::queries_type == 1) return read_spydrpick_input();
        if (ProgramOptions::queries_type == 2) return read_ud_output();
        return Queries();
    }

private:
    static void print_error(const std::string& line, int_t n_columns) {
        std::cerr << "Not enough columns (" << n_columns << " required) in queries file \"" << po::queries_filename
                  << "\" line \"" << line << "\". Is the file space-separated?" << std::endl;
    }
    static Queries read_general_input() {
        Queries queries(po::queries_type);
        std::ifstream ifs(po::queries_filename);
        int_t cnt = 0;

        for (std::string line; std::getline(ifs, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 2) {
                print_error(line, 5);
                return Queries();
            }
            int_t v = std::stoll(fields[0]) - po::queries_one_based;
            int_t w = std::stoll(fields[1]) - po::queries_one_based;
            queries.emplace_back(v, w);

            if (++cnt == ProgramOptions::n_queries) break;
        }

        return queries;
    }

    static Queries read_spydrpick_input() {
        Queries queries(po::queries_type);
        std::ifstream ifs(po::queries_filename);
        int_t cnt = 0;

        for (std::string line; std::getline(ifs, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 5) {
                print_error(line, 6);
                return Queries();
            }
            int_t v = std::stoll(fields[0]) - po::queries_one_based;
            int_t w = std::stoll(fields[1]) - po::queries_one_based;
            // Ignore SpydrPick's 3rd field.
            bool flag = std::stoi(fields[3]);
            real_t score = std::stod(fields[4]);
            queries.emplace_back(v, w, flag, score);

            if (++cnt == ProgramOptions::n_queries) break;
        }

        return queries;
    }

    static Queries read_ud_output() {
        Queries queries(po::queries_type);
        std::ifstream ifs(po::queries_filename);
        int_t cnt = 0;

        for (std::string line; std::getline(ifs, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 5) {
                print_error(line, 5);
                return Queries();
            }
            int_t v = std::stoll(fields[0]) - po::queries_one_based;
            int_t w = std::stoll(fields[1]) - po::queries_one_based;
            int_t distance = std::stoll(fields[2]);
            bool flag = std::stoi(fields[3]);
            real_t score = std::stod(fields[4]);
            int_t count = std::stoll(fields[5]);
            queries.emplace_back(v, w, flag, score, distance, count);

            if (++cnt == ProgramOptions::n_queries) break;
        }

        return queries;
    }
};

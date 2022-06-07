#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "DistanceVector.hpp"
#include "Queries.hpp"
#include "Utils.hpp"

class QueriesReader {
public:
    static Queries read_queries(const std::string& queries_filename, int_t n_queries, int_t queries_type, bool queries_one_based = false) {
        if (queries_type == 0) return read_general_input(queries_filename, n_queries, queries_type, queries_one_based);
        if (queries_type == 1) return read_spydrpick_input(queries_filename, n_queries, queries_type, queries_one_based);
        if (queries_type == 2) return read_ud_output(queries_filename, n_queries, queries_type, queries_one_based);
        return Queries();
    }

private:
    static void print_error(const std::string& queries_filename, const std::string& line, int_t n_columns) {
        std::cerr << "Not enough columns (" << n_columns << " required) in queries file \"" << queries_filename
                  << "\" line \"" << line << "\". Is the file space-separated?" << std::endl;
    }
    static Queries read_general_input(const std::string& queries_filename, int_t n_queries, int_t queries_type, bool queries_one_based) {
        Queries queries(queries_type);
        std::ifstream ifs(queries_filename);
        int_t cnt = 0;

        for (std::string line; std::getline(ifs, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 2) {
                print_error(queries_filename, line, 5);
                return Queries();
            }
            int_t v = std::stoll(fields[0]) - queries_one_based;
            int_t w = std::stoll(fields[1]) - queries_one_based;
            queries.emplace_back(v, w);

            if (++cnt == n_queries) break;
        }

        return queries;
    }

    static Queries read_spydrpick_input(const std::string& queries_filename, int_t n_queries, int_t queries_type, bool queries_one_based) {
        Queries queries(queries_type);
        std::ifstream ifs(queries_filename);
        int_t cnt = 0;

        for (std::string line; std::getline(ifs, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 5) {
                print_error(queries_filename, line, 6);
                return Queries();
            }
            int_t v = std::stoll(fields[0]) - queries_one_based;
            int_t w = std::stoll(fields[1]) - queries_one_based;
            // Ignore SpydrPick's 3rd field.
            bool flag = std::stoi(fields[3]);
            real_t score = std::stod(fields[4]);
            queries.emplace_back(v, w, flag, score);

            if (++cnt == n_queries) break;
        }

        return queries;
    }

    static Queries read_ud_output(const std::string& queries_filename, int_t n_queries, int_t queries_type, bool queries_one_based) {
        Queries queries(queries_type);
        std::ifstream ifs(queries_filename);
        int_t cnt = 0;

        for (std::string line; std::getline(ifs, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 5) {
                print_error(queries_filename, line, 5);
                return Queries();
            }
            int_t v = std::stoll(fields[0]) - queries_one_based;
            int_t w = std::stoll(fields[1]) - queries_one_based;
            int_t distance = std::stoll(fields[2]);
            bool flag = std::stoi(fields[3]);
            real_t score = std::stod(fields[4]);
            int_t count = std::stoll(fields[5]);
            queries.emplace_back(v, w, flag, score, distance, count);

            if (++cnt == n_queries) break;
        }

        return queries;
    }
};

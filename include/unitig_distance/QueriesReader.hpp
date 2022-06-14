#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "DistanceVector.hpp"
#include "Queries.hpp"
#include "Utils.hpp"

// Class which handles reading queries. Supports process substitution input.
class QueriesReader {
public:
    static Queries read_queries() {
        Queries queries;

        std::ifstream ifs(ProgramOptions::queries_filename);
        std::string line;
        std::getline(ifs, line);
        auto fields_sz = Utils::get_fields(line).size();

        if (fields_sz == 2) {
            read_general_input_line(queries, line, 1);
            if (!read_general_input(queries, ifs)) return Queries();
        }
        if (fields_sz == 5) {
            read_spydrpick_input_line(queries, line, 1);
            if (!read_spydrpick_input(queries, ifs)) return Queries();
        }
        if (fields_sz == 6) {
            read_ud_input_line(queries, line, 1);
            if (!read_ud_input(queries, ifs)) return Queries();
        }
        return queries;
    }

private:
    static void print_error(const std::string& line, int_t n_columns, int_t count) {
        std::cerr << "Not enough columns (" << n_columns << " required) in queries file \"" << ProgramOptions::queries_filename
                  << "\" line " << count << " \"" << line << "\". Is the file space-separated?" << std::endl;
    }

    static bool read_general_input_line(Queries& queries, const std::string& line, int_t cnt) {
        auto fields = Utils::get_fields(line);
        if (fields.size() < 2) {
            print_error(line, 2, cnt);
            return false;
        }
        int_t v = std::stoll(fields[0]) - ProgramOptions::queries_one_based;
        int_t w = std::stoll(fields[1]) - ProgramOptions::queries_one_based;
        queries.add_vertices(v, w);
        return true;
    }

    static bool read_spydrpick_input_line(Queries& queries, const std::string& line, int_t cnt) {
        auto fields = Utils::get_fields(line);
        if (fields.size() < 5) {
            print_error(line, 5, cnt);
            return false;
        }
        int_t v = std::stoll(fields[0]) - ProgramOptions::queries_one_based;
        int_t w = std::stoll(fields[1]) - ProgramOptions::queries_one_based;
        // Ignore SpydrPick's 3rd field.
        bool flag = std::stoi(fields[3]);
        real_t score = std::stod(fields[4]);
        queries.add_vertices(v, w);
        queries.add_flag(flag);
        queries.add_score(score);
        return true;
    }

    static bool read_ud_input_line(Queries& queries, const std::string& line, int_t cnt) {
        auto fields = Utils::get_fields(line);
        if (fields.size() < 6) {
            print_error(line, 6, cnt);
            return false;
        }
        int_t v = std::stoll(fields[0]) - ProgramOptions::queries_one_based;
        int_t w = std::stoll(fields[1]) - ProgramOptions::queries_one_based;
        int_t distance = std::stoll(fields[2]);
        bool flag = std::stoi(fields[3]);
        real_t score = std::stod(fields[4]);
        int_t count = std::stoll(fields[5]);
        queries.add_vertices(v, w);
        queries.add_flag(flag);
        queries.add_score(score);
        queries.add_distance(distance, count);
        return true;
    }

    static bool read_general_input(Queries& queries, std::ifstream& ifs) {
        int_t cnt = 1;
        for (std::string line; ++cnt <= ProgramOptions::n_queries && std::getline(ifs, line); ) {
            if (!read_general_input_line(queries, line, cnt)) return false;
        }
        return true;
    }

    static bool read_spydrpick_input(Queries& queries, std::ifstream& ifs) {
        int_t cnt = 1;
        for (std::string line; ++cnt <= ProgramOptions::n_queries && std::getline(ifs, line); ) {
            if (!read_spydrpick_input_line(queries, line, cnt)) return false;
        }
        return true;
    }

    static bool read_ud_input(Queries& queries, std::ifstream& ifs) {
        int_t cnt = 1;
        for (std::string line; ++cnt <= ProgramOptions::n_queries && std::getline(ifs, line); ) {
            if (!read_ud_input_line(queries, line, cnt)) return false;
        }
        return true;
    }
};

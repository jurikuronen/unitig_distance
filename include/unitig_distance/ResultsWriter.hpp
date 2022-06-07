#pragma once

#include <fstream>

#include "DistanceVector.hpp"
#include "Queries.hpp"
#include "Utils.hpp"

class ResultsWriter {
public:
    static void output_results(const std::string& out_filename, const Queries& queries, const DistanceVector& dv, bool output_one_based = false, real_t max_distance = REAL_T_MAX) {
        if (queries.queries_type() == 0) {
            output_general_results(out_filename, queries, dv, output_one_based, max_distance);
        } else {
            std::ofstream ofs(out_filename);
            for (std::size_t i = 0; i < queries.size(); ++i) {
                ofs << queries.v(i) + output_one_based << ' '
                    << queries.w(i) + output_one_based << ' '
                    << (int_t) Utils::fixed_distance(dv[i].distance(), max_distance) << ' '
                    << queries.flag(i) << ' '
                    << queries.score(i) << ' '
                    << dv[i].count() << '\n';
            }
        }
    }

private:
    static void output_general_results(const std::string& out_filename, const Queries& queries, const DistanceVector& dv, bool output_one_based, real_t max_distance) {
        std::ofstream ofs(out_filename);
        for (std::size_t i = 0; i < queries.size(); ++i) {
            ofs << queries.v(i) + output_one_based << ' '
                << queries.w(i) + output_one_based << ' '
                << (int_t) Utils::fixed_distance(dv[i].distance(), max_distance) << ' '
                << dv[i].count() << '\n';
        }
    }

};

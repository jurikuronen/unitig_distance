#pragma once

#include <fstream>

#include "DistanceVector.hpp"
#include "ProgramOptions.hpp"
#include "Queries.hpp"
#include "Utils.hpp"

class ResultsWriter {
public:
    static void output_results(const std::string& out_filename, const Queries& queries, const DistanceVector& dv) {
        std::ofstream ofs(out_filename);
        if (queries.queries_type() == 0) {
            for (std::size_t i = 0; i < queries.size(); ++i) {
                ofs << queries.v(i) + ProgramOptions::output_one_based << ' '
                    << queries.w(i) + ProgramOptions::output_one_based << ' '
                    << (int_t) Utils::fixed_distance(dv[i].distance(), ProgramOptions::max_distance) << ' '
                    << dv[i].count() << '\n';
            }
        } else {
            for (std::size_t i = 0; i < queries.size(); ++i) {
                ofs << queries.v(i) + ProgramOptions::output_one_based << ' '
                    << queries.w(i) + ProgramOptions::output_one_based << ' '
                    << (int_t) Utils::fixed_distance(dv[i].distance(), ProgramOptions::max_distance) << ' '
                    << queries.flag(i) << ' '
                    << queries.score(i) << ' '
                    << dv[i].count() << '\n';
            }
        }
    }

    static void output_results(const std::string& out_filename, const Queries& queries, const DistanceVector& dv, const std::vector<int_t>& indices) {
        std::ofstream ofs(out_filename);
        if (queries.queries_type() == 0) {
            for (auto i : indices) {
                ofs << queries.v(i) + ProgramOptions::output_one_based << ' '
                    << queries.w(i) + ProgramOptions::output_one_based << ' '
                    << (int_t) Utils::fixed_distance(dv[i].distance(), ProgramOptions::max_distance) << ' '
                    << dv[i].count() << '\n';
            }
        } else {
            for (auto i : indices) {
                ofs << queries.v(i) + ProgramOptions::output_one_based << ' '
                    << queries.w(i) + ProgramOptions::output_one_based << ' '
                    << (int_t) Utils::fixed_distance(dv[i].distance(), ProgramOptions::max_distance) << ' '
                    << queries.flag(i) << ' '
                    << queries.score(i) << ' '
                    << dv[i].count() << '\n';
            }
        }
    }

};

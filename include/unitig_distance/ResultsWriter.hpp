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
        bool write_counts = dv.storing_mean_distances();
        bool extended = queries.extended_format();
        if (extended && write_counts) output_extended_with_count(ofs, queries, dv);
        else if (extended && !write_counts) output_extended(ofs, queries, dv);
        else if (!extended && write_counts) output_simple_with_count(ofs, queries, dv);
        else output_simple(ofs, queries, dv);
    }

    static void output_results(const std::string& out_filename, const Queries& queries, const DistanceVector& dv, const std::vector<int_t>& indices) {
        std::ofstream ofs(out_filename);
        bool write_counts = dv.storing_mean_distances();
        bool extended = queries.extended_format();
        if (extended && write_counts) output_extended_with_count(ofs, queries, dv, indices);
        else if (extended && !write_counts) output_extended(ofs, queries, dv, indices);
        else if (!extended && write_counts) output_simple_with_count(ofs, queries, dv, indices);
        else output_simple(ofs, queries, dv, indices);
    }

private:
    static void output_extended_line(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv, int_t idx) {
        ofs << queries.v(idx) + ProgramOptions::output_one_based << ' '
            << queries.w(idx) + ProgramOptions::output_one_based << ' '
            << (int_t) Utils::fixed_distance(dv[idx].distance(), ProgramOptions::max_distance) << ' '
            << queries.flag(idx) << ' '
            << queries.score(idx) << '\n';
    }

    static void output_simple_line(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv, int_t idx) {
        ofs << queries.v(idx) + ProgramOptions::output_one_based << ' '
            << queries.w(idx) + ProgramOptions::output_one_based << ' '
            << (int_t) Utils::fixed_distance(dv[idx].distance(), ProgramOptions::max_distance) << '\n';
    }

    static void output_extended_line_with_count(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv, int_t idx) {
        ofs << queries.v(idx) + ProgramOptions::output_one_based << ' '
            << queries.w(idx) + ProgramOptions::output_one_based << ' '
            << (int_t) Utils::fixed_distance(dv[idx].distance(), ProgramOptions::max_distance) << ' '
            << queries.flag(idx) << ' '
            << queries.score(idx) << ' '
            << dv[idx].count() << '\n';
    }

    static void output_simple_line_with_count(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv, int_t idx) {
        ofs << queries.v(idx) + ProgramOptions::output_one_based << ' '
            << queries.w(idx) + ProgramOptions::output_one_based << ' '
            << (int_t) Utils::fixed_distance(dv[idx].distance(), ProgramOptions::max_distance) << ' '
            << dv[idx].count() << '\n';
    }

    static void output_extended(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv, const std::vector<int_t>& indices) {
        for (auto i : indices) output_extended_line(ofs, queries, dv, i);
    }

    static void output_simple(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv, const std::vector<int_t>& indices) {
        for (auto i : indices) output_simple_line(ofs, queries, dv, i);
    }

    static void output_extended_with_count(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv, const std::vector<int_t>& indices) {
        for (auto i : indices) output_extended_line_with_count(ofs, queries, dv, i);
    }

    static void output_simple_with_count(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv, const std::vector<int_t>& indices) {
        for (auto i : indices) output_simple_line_with_count(ofs, queries, dv, i);
    }

    static void output_extended(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv) {
        for (std::size_t i = 0; i < queries.size(); ++i) output_extended_line(ofs, queries, dv, i);
    }

    static void output_simple(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv) {
        for (std::size_t i = 0; i < queries.size(); ++i) output_simple_line(ofs, queries, dv, i);
    }

    static void output_extended_with_count(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv) {
        for (std::size_t i = 0; i < queries.size(); ++i) output_extended_line_with_count(ofs, queries, dv, i);
    }

    static void output_simple_with_count(std::ofstream& ofs, const Queries& queries, const DistanceVector& dv) {
        for (std::size_t i = 0; i < queries.size(); ++i) output_simple_line_with_count(ofs, queries, dv, i);
    }
};

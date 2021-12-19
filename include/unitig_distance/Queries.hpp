#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "types.hpp"
#include "unitig_distance.hpp"

class Queries {
public:
    // Read queries file.
    Queries(const std::string& queries_filename, int_t n_queries, bool queries_one_based = false, bool output_one_based = false, real_t max_distance = REAL_T_MAX)
    : m_output_one_based(output_one_based), m_max_distance(max_distance), m_largest_v(0), m_largest_score(0.0)
    {
        std::ifstream ifs(queries_filename);
        int_t cnt = 0;
        for (std::string line; std::getline(ifs, line); ) {
            auto fields = unitig_distance::get_fields(line);
            if (fields.size() < 2) {
                std::cout << "Wrong number of fields in queries file: " << queries_filename << std::endl;
                m_queries.clear();
                return;
            }
            int_t v = std::stoll(fields[0]) - queries_one_based;
            int_t w = std::stoll(fields[1]) - queries_one_based;
            m_largest_v = std::max(m_largest_v, std::max(v, w));
            m_queries.emplace_back(v, w);
            if (fields.size() > 2) {
                // Queries file is the output of another program.
                real_t score = std::stod(fields[4]); // Score must be in column 5.
                m_largest_score = std::max(m_largest_score, score);
                m_scores.push_back(score);
            }
            m_fields.push_back(std::move(fields));
            if (++cnt == n_queries) break;
        }
    }

    // Create a copy with given indices.
    Queries(const Queries& other, const std::vector<int_t>& indices) : m_output_one_based(other.m_output_one_based), m_max_distance(other.m_max_distance) {
        for (auto i : indices) {
            m_queries.emplace_back(other.v(i), other.w(i));
            m_scores.push_back(other.score(i));
            m_fields.push_back(other.m_fields[i]);
        }
    }

    // Output queries back with the calculated distances.
    void output_distances(const std::string& out_filename, const std::vector<real_t>& distances) const {
        std::ofstream ofs(out_filename);
        for (std::size_t i = 0; i < size(); ++i) {
            int_t distance = (int_t) unitig_distance::fixed_distance(distances[i], m_max_distance);
            ofs << v(i) + m_output_one_based << ' ' << w(i) + m_output_one_based << ' ' << distance;
            for (std::size_t field_idx = 3; field_idx < m_fields[i].size(); ++field_idx) {
                ofs << ' ' << m_fields[i][field_idx];
            }
            ofs << '\n';
        }
    }

    // Output counts of connected query vertices in the single genome graphs.
    void output_counts(const std::string& out_filename, const std::vector<int_t>& counts) const {
        std::ofstream ofs(out_filename);
        for (std::size_t i = 0; i < size(); ++i) {
            ofs << v(i) + m_output_one_based << ' ' << w(i) + m_output_one_based << ' ' << counts[i];
            for (std::size_t field_idx = 3; field_idx < m_fields[i].size(); ++field_idx) {
                ofs << ' ' << m_fields[i][field_idx];
            }
            ofs << '\n';
        }
    }

    std::size_t size() const { return m_queries.size(); }

    int_t v(std::size_t idx) const { return m_queries[idx].first; }
    int_t w(std::size_t idx) const { return m_queries[idx].second; }
    real_t score(std::size_t idx) const { return m_scores[idx]; }

    int_t largest_v() const { return m_largest_v; }
    real_t largest_score() const { return m_largest_score; }

    std::vector<real_t> get_distance_vector() const {
        std::vector<real_t> distances;
        for (const auto& fields : m_fields) distances.push_back(std::stod(fields[2]));
        return distances;
    }

    bool using_extended_queries() const { return m_scores.size() > 0; }

    typename std::vector<std::pair<int_t, int_t>>::iterator begin() { return m_queries.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::iterator end() { return m_queries.end(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator begin() const { return m_queries.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator end() const { return m_queries.end(); }

private:
    std::vector<std::pair<int_t, int_t>> m_queries;
    std::vector<real_t> m_scores;
    std::vector<std::vector<std::string>> m_fields;

    bool m_output_one_based;

    real_t m_max_distance;

    int_t m_largest_v;
    real_t m_largest_score;

};


#pragma once

#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "types.hpp"
#include "unitig_distance.hpp"

class Queries {
public:
    // Read queries file.
    Queries(const std::string& queries_filename, int_t n, bool one_based = false, real_t max_distance = REAL_T_MAX) : m_max_distance(max_distance) {
        std::ifstream ifs(queries_filename);
        int_t cnt = 0;
        for (std::string line; std::getline(ifs, line); ) {
            auto fields = unitig_distance::get_fields(line);
            if (fields.size() < 2) {
                m_queries.clear();
                return;
            }
            int_t v = std::stoll(fields[0]) - one_based;
            int_t w = std::stoll(fields[1]) - one_based;
            m_queries.emplace_back(v, w);
            if (fields.size() > 2) {
                // Queries file is the output of another program.
                real_t score = std::stod(fields[4]); // Score must be in column 5.
                m_scores.push_back(score);
            }
            m_fields.push_back(std::move(fields));
            if (++cnt == n) break;
        }
    }

    // Output queries back with the calculated distances.
    void output_distances(const std::string& out_filename, const std::vector<real_t>& distances) const {
        std::ofstream ofs(out_filename);
        for (std::size_t i = 0; i < size(); ++i) {
            int_t distance = unitig_distance::fixed_distance(distances[i], m_max_distance);
            ofs << m_fields[i][0] << ' ' << m_fields[i][1] << ' ' << distance;
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
            ofs << m_fields[i][0] << ' ' << m_fields[i][1] << ' ' << counts[i];
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

    typename std::vector<std::pair<int_t, int_t>>::iterator begin() { return m_queries.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::iterator end() { return m_queries.end(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator begin() const { return m_queries.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator end() const { return m_queries.end(); }

private:
    std::vector<std::pair<int_t, int_t>> m_queries;
    std::vector<real_t> m_scores;
    std::vector<std::vector<std::string>> m_fields;

    real_t m_max_distance;

};


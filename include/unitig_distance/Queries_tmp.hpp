#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "types.hpp"
#include "Utils.hpp"

// Class for storing queries and results.
class Queries {
public:
    Queries() : m_largest_v(-1), m_n_vs(-1), m_largest_score(-1.0) { }
    // Read queries file.
    Queries(const std::string& queries_filename, int_t n_queries, int_t queries_type, bool queries_one_based = false)
        : m_largest_v(-1), m_n_vs(-1), m_largest_score(-1.0)
    {
        std::ifstream ifs(queries_filename);
        int_t cnt = 0;
        for (std::string line; std::getline(ifs, line); ) {
            auto fields = Utils::get_fields(line);

            // Input: (v, w, *, flag, score) (SpydrPick)
            if (queries_type == 1)
                m_flags.emplace_back(std::stoi(fields[3]));
                m_scores.emplace_back(std::stod(fields[4]));
            }

            // Input: (v, w, distance, flag, score, count) (unitig_distance)
            else if (queries_type == 2) {

            }

            // Input: (v, w, score) (any pairwise score based method)
            else if (queries_type == 3) {
                if (fields.size() < 3) {
                    print_error(queries_filename, line, 3);
                    return;
                }
                m_scores.emplace_back(std::stod(fields[2]));
            }

            // Input: (v, w, flag, score) (any pairwise score based method with flag provided)
            else if (queries_type == 4) {
                m_flags.emplace_back(std::stoi(fields[2]));
                m_scores.emplace_back(std::stod(fields[3]));
            }

            // Extended queries have 5-6 columns.
            if (fields.size() >= 5) {
                m_flags.emplace_back(std::stoi(fields[3])); // Flag must be in column 4.
                m_scores.emplace_back(std::stod(fields[4])); // Score must be in column 5.
                // Distance and count are in columns 3 and 6.
                if (fields.size() >= 6) {
                    m_distances.emplace_back(std::stoll(fields[2]), std::stoll(fields[6]));
                }
            }

            if (fields.size() < 2) {
                print_error(queries_filename, line, 2);
                return;
            }

            // v and w must be in columns 1-2.
            int_t v = std::stoll(fields[0]) - queries_one_based;
            int_t w = std::stoll(fields[1]) - queries_one_based;
            m_queries.emplace_back(v, w);

            if (++cnt == n_queries) break;
        }

        m_distances.resize(cnt);
    }

    // Create a copy with given indices.
    Queries(const Queries& other, const std::vector<int_t>& indices) {
        for (auto i : indices) {
            m_queries.emplace_back(other.v(i), other.w(i));
            m_distances.emplace_back(other.distance(i));
            m_flags.push_back(other.flag(i));
            m_scores.push_back(other.score(i));
        }
    }

    void output_results(const std::string& out_filename, bool one_based = false, real_t max_distance = REAL_T_MAX) {
        std::ofstream ofs(out_filename);
        for (std::size_t i = 0; i < results.size(); ++i) {
            ofs << v(i) + one_based << ' '
                << w(i) + one_based << ' '
                << (int_t) Utils::fixed_distance(distance(i), max_distance) << ' '
                << flag(i) << ' '
                << score(i) << ' '
                << count(i) << '\n';
        }
    }

    int_t v(std::size_t idx) { return m_queries[idx].first; }
    int_t w(std::size_t idx) { return m_queries[idx].second; }
    int_t distance(std::size_t idx) { return m_distances[idx].distance; }
    int_t count(std::size_t idx) { return m_distances[idx].count; }
    bool flag(std::size_t idx) { return m_flags[idx]; }
    real_t score(std::size_t idx) { return m_scores[idx]; }
    
    void gather_info() {
        std::unordered_set<int_t> vs;
        for (auto q : *this) {
            int_t v, w;
            std::tie(v, w) = q;
            vs.insert(v);
            vs.insert(w);
            m_largest_v = std::max(m_largest_v, std::max(v, w));
        }
        m_n_vs = vs.size();
        
        for (auto score : m_scores) {
            m_largest_score = std::max(m_largest_score, score);
        }
    }

    int_t largest_v() const { return m_largest_v; }
    int_t n_vs() const { return m_n_vs; }
    real_t largest_score() const { return m_largest_score; }

    std::vector<real_t> distances() const { return m_distances.distances(); }
    std::vector<int_t> counts() const { return m_distances.counts(); }
    
    std::vector<real_t> get_distance_vector() const {
        std::vector<real_t> distances;
        for (const auto& fields : m_fields) distances.push_back(std::stod(fields[2]));
        return distances;
    }

    bool using_extended_queries() const { return m_scores.size() > 0; }

    std::size_t size() const { return m_queries.size(); }

    typename std::vector<std::pair<int_t, int_t>>::iterator begin() { return m_queries.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::iterator end() { return m_queries.end(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator begin() const { return m_queries.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator end() const { return m_queries.end(); }

private:
    std::vector<std::pair<int_t, int_t>> m_queries;
    DistanceVector m_distances;
    std::vector<bool> m_flags;
    std::vector<real_t> m_scores;

    int_t m_largest_v;
    int_t m_n_vs;
    real_t m_largest_score;

};


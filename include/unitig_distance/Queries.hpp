#pragma once

#include <utility>
#include <vector>

#include "Distance.hpp"
#include "DistanceVector.hpp"
#include "types.hpp"

// Class for storing queries read from input. Not all fields may be available.
class Queries {
public:
    Queries() : m_largest_v(-1) { }

    std::size_t size() const { return m_queries.size(); }

    int_t largest_v() const { return m_largest_v; }

    const DistanceVector& distances() const { return m_distances; }

    int_t v(std::size_t idx) const { return m_queries[idx].first; }
    int_t w(std::size_t idx) const { return m_queries[idx].second; }
    bool flag(std::size_t idx) const { return m_flags[idx]; }
    real_t score(std::size_t idx) const { return m_scores[idx]; }
    real_t distance(std::size_t idx) const { return m_distances[idx].distance(); }
    int_t count(std::size_t idx) const { return m_distances[idx].count(); }

    void emplace_back(int_t v, int_t w) {
        m_queries.emplace_back(v, w);
        m_largest_v = std::max(m_largest_v, std::max(v, w));
    }
    void emplace_back(int_t v, int_t w, real_t score) {
        emplace_back(v, w);
        m_scores.emplace_back(score);
    }
    void emplace_back(int_t v, int_t w, bool flag, real_t score) {
        emplace_back(v, w, score);
        m_flags.emplace_back(flag);
    }
    void emplace_back(int_t v, int_t w, bool flag, real_t score, int_t distance, int_t count) {
        emplace_back(v, w, flag, score);
        m_distances.emplace_back(distance, count);
    }

    bool extended_format() const { return m_scores.size() > 0; } 

    //typename std::vector<std::pair<int_t, int_t>>::iterator begin() { return m_queries.begin(); }
    //typename std::vector<std::pair<int_t, int_t>>::iterator end() { return m_queries.end(); }
    //typename std::vector<std::pair<int_t, int_t>>::const_iterator begin() const { return m_queries.begin(); }
    //typename std::vector<std::pair<int_t, int_t>>::const_iterator end() const { return m_queries.end(); }

private:
    std::vector<std::pair<int_t, int_t>> m_queries;
    std::vector<bool> m_flags;
    std::vector<real_t> m_scores;
    DistanceVector m_distances;

    int_t m_largest_v;

};


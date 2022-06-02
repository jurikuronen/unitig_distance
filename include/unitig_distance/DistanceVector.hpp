#pragma once

#include <algorithm>
#include <vector>

#include "Distance.hpp"
#include "types.hpp"

class DistanceVector {
public:
    DistanceVector() = delete;
    DistanceVector(std::size_t sz) : m_distances(sz) { }
    DistanceVector(std::size_t sz, real_t distance_value) : m_distances(sz, Distance(distance_value)) { }
    DistanceVector(std::size_t sz, real_t distance_value, int_t count_value) : m_distances(sz, Distance(distance_value, count_value)) { }

    std::vector<real_t> distances() const {
        std::vector<real_t> vector(size());
        static auto get_distance = [](const Distance& distance) { return distance.distance(); };
        std::transform(m_distances.begin(), m_distances.end(), vector.begin(), get_distance);
        return vector;
    }

    std::vector<int_t> counts() const {
        std::vector<int_t> vector(size());
        static auto get_count = [](const Distance& distance) { return distance.count(); };
        std::transform(m_distances.begin(), m_distances.end(), vector.begin(), get_count);
        return vector;
    }

    std::size_t size() const { return m_distances.size(); }

    Distance& operator[](std::size_t idx) { return m_distances[idx]; }

    typename std::vector<Distance>::iterator begin() { return m_distances.begin(); }
    typename std::vector<Distance>::iterator end() { return m_distances.end(); }
    typename std::vector<Distance>::const_iterator begin() const { return m_distances.begin(); }
    typename std::vector<Distance>::const_iterator end() const { return m_distances.end(); }

private:
    std::vector<Distance> m_distances;

};

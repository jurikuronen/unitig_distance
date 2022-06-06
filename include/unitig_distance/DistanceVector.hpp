#pragma once

#include <algorithm>
#include <functional>
#include <vector>

#include "Distance.hpp"
#include "types.hpp"

class DistanceVector {
public:
    DistanceVector(std::size_t sz) : m_distances(sz) { }
    DistanceVector(std::size_t sz, real_t distance_value) : m_distances(sz, Distance(distance_value)) { }
    DistanceVector(std::size_t sz, real_t distance_value, int_t count_value) : m_distances(sz, Distance(distance_value, count_value)) { }

    std::vector<real_t> distances() const {
        std::vector<real_t> vector(size());
        std::transform(begin(), end(), vector.begin(), std::mem_fn(&Distance::distance));
        return vector;
    }

    std::vector<int_t> counts() const {
        std::vector<int_t> vector(size());
        std::transform(begin(), end(), vector.begin(), std::mem_fn(&Distance::count));
        return vector;
    }

    void resize(std::size_t sz) { m_distances.resize(sz); }

    std::size_t size() const { return m_distances.size(); }

    Distance& operator[](std::size_t idx) { return m_distances[idx]; }

    typename std::vector<Distance>::iterator begin() { return m_distances.begin(); }
    typename std::vector<Distance>::iterator end() { return m_distances.end(); }
    typename std::vector<Distance>::const_iterator begin() const { return m_distances.begin(); }
    typename std::vector<Distance>::const_iterator end() const { return m_distances.end(); }

private:
    std::vector<Distance> m_distances;

};

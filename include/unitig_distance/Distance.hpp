#pragma once

#include "types.hpp"

class Distance {
public:
    Distance() : Distance(0.0, 0) { }
    Distance(real_t distance) : Distance(distance, 1) { }
    Distance(real_t distance, int_t count) : m_distance(distance), m_count(count) { }

    real_t distance() const { return m_distance; }
    int_t count() const { return m_count; }

    Distance operator+(const Distance& other) {
        return Distance((distance() * count() + other.distance() * other.count()) / (count() + other.count()), count() + other.count());
    }
    Distance& operator+=(const Distance& other) {
        return *this = *this + other;
    }
    operator real_t() const { return m_distance; }

private:
    real_t m_distance;
    int_t m_count;

};

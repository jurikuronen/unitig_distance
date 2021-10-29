#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "types.hpp"

using neighbor_t = std::pair<std::size_t, real_t>;
using neighbor_itr_t = typename std::vector<neighbor_t>::iterator;
using neighbor_const_itr_t = typename std::vector<neighbor_t>::const_iterator;

// Data structure for a Node and its neighbor nodes.
class Node {
public:
    Node() { }

    std::vector<neighbor_t>& neighbors() { return m_neighbors; }
    const std::vector<neighbor_t>& neighbors() const { return m_neighbors; }

    void add_neighbor(std::size_t neighbor_idx, real_t weight) { m_neighbors.emplace_back(neighbor_idx, weight); }
    bool has_neighbor(std::size_t neighbor_idx) const { return find_neighbor(neighbor_idx) != end(); }
    void remove_neighbor(std::size_t neighbor_idx) { auto it = find_neighbor(neighbor_idx); if (it != end()) m_neighbors.erase(it); }

    real_t get_largest_weight() const {
        real_t largest_weight = 0.0;
        for (auto it = begin(); it != end(); ++it) largest_weight = std::max(largest_weight, it->second);
        return largest_weight;
    }
    int_t degree() const { return m_neighbors.size(); }

    neighbor_itr_t begin() { return m_neighbors.begin(); }
    neighbor_itr_t end() { return m_neighbors.end(); }
    neighbor_const_itr_t begin() const { return m_neighbors.begin(); }
    neighbor_const_itr_t end() const { return m_neighbors.end(); }

    neighbor_itr_t find_neighbor(std::size_t neighbor_idx) { auto it = begin(); while (it != end() && it->first != neighbor_idx) ++it; return it; }
    neighbor_const_itr_t find_neighbor(std::size_t neighbor_idx) const { auto it = begin(); while (it != end() && it->first != neighbor_idx) ++it; return it; }

private:
    std::vector<neighbor_t> m_neighbors; // Stores (neighbor_idx, edge_weight) pairs.

};


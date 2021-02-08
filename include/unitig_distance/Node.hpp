/*
    Node data structure.
    Stores an arbitrary id, weight and component id.
    The node is accessed by the graph through its index, only known by the graph.
    However, the neighbors vector stores the indices of this node's neighbors in the graph
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "types.hpp"

class Node {
public:
    Node() : Node(-1, 1, -1) { }
    Node(int_t id) : Node(id, 1, -1) { }
    Node(int_t id, int_t weight) : Node(id, weight, -1) { }
    Node(int_t id, int_t weight, int_t component_id) : m_id(id), m_weight(weight), m_component_id(component_id), m_is_articulation_point(false), m_is_cut_node(false) { }

    int_t id() const { return m_id; }
    int_t weight() const { return m_weight; }
    int_t component_id() const { return m_component_id; }
    std::vector<int_t>& neighbors() { return m_neighbors; }
    const std::vector<int_t>& neighbors() const { return m_neighbors; }
    bool is_articulation_point() const { return m_is_articulation_point; } // Node separates the graph into a biconnected component.
    bool is_cut_node() const { return m_is_cut_node; } // Node is part of a pair separating the graph into a triconnected component.
    bool is_articulation_point_or_cut_node() const { return is_articulation_point() || is_cut_node(); }

    void set_id(int_t id) { m_id = id; }
    void set_weight(int_t weight) { m_weight = weight; }
    void set_component_id(int_t component_id) { m_component_id = component_id; }
    bool has_neighbor(int_t neighbor_idx) const { return find_neighbor_iterator(neighbor_idx) != end(); }
    void add_neighbor(int_t neighbor_idx) { if (!has_neighbor(neighbor_idx)) m_neighbors.push_back(neighbor_idx); }
    void remove_neighbor(int_t neighbor_idx) { auto it = find_neighbor_iterator(neighbor_idx); if (it != end()) m_neighbors.erase(it); }
    void sort_neighbors() { std::sort(begin(), end()); }
    void set_articulation_point() { m_is_articulation_point = true; }
    void unset_articulation_point() { m_is_articulation_point = false; }
    void set_cut_node() { m_is_cut_node = true; }
    void unset_cut_node() { m_is_cut_node = false; }

    typename std::vector<int_t>::iterator begin() { return m_neighbors.begin(); }
    typename std::vector<int_t>::iterator end() { return m_neighbors.end(); }
    typename std::vector<int_t>::const_iterator begin() const { return m_neighbors.begin(); }
    typename std::vector<int_t>::const_iterator end() const { return m_neighbors.end(); }
    typename std::vector<int_t>::iterator find_neighbor_iterator(int_t neighbor_idx) { return std::find(begin(), end(), neighbor_idx); }
    typename std::vector<int_t>::const_iterator find_neighbor_iterator(int_t neighbor_idx) const { return std::find(begin(), end(), neighbor_idx); }

private:
    int_t m_id; // Arbitrary, default -1.
    int_t m_weight; // Default 1.
    int_t m_component_id; // Default -1: not assigned to a component.
    bool m_is_articulation_point;
    bool m_is_cut_node;
    std::vector<int_t> m_neighbors;

};

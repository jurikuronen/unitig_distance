#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Node.hpp"
#include "types.hpp"

using node_itr_t = typename std::vector<Node>::iterator;
using node_const_itr_t = typename std::vector<Node>::const_iterator;

class Graph {
public:
    Graph() { }

    // Copy constructor.
    Graph(const Graph& graph);

    // Construct the combined graph that stores two sides for each node: one for its left side and one for its right side, considered from the canonical form.
    Graph(const std::string& nodes_filename, const std::string& edges_filename, int_t kmer_length);

    // Construct an edge-induced subgraph from the combined graph.
    Graph(const Graph& combined_graph, const std::string& edges_filename);

    // Construct a filtered subgraph from the combined graph.
    Graph(const Graph& combined_graph, const std::string& filter_filename, int_t criterion);

    void add_node() { m_nodes.emplace_back(); }
    void add_edge(std::size_t idx_1, std::size_t idx_2, real_t weight);
    void remove_neighbors(std::size_t idx);
    void remove_edge(std::size_t idx_1, std::size_t idx_2);

    std::size_t size() const { return m_nodes.size(); }

    // Useful functions if graph stores two sides for each node.
    std::size_t true_size() const { return size() / 2; }
    std::size_t left_node(int_t node_id) const { return node_id * 2; }
    std::size_t right_node(int_t node_id) const { return node_id * 2 + 1; }
    int_t idx_to_id(std::size_t idx) const { return idx / 2; }

    // Node one-basedness.
    bool one_based() const { return m_one_based; }
    void set_one_based(bool one_based) { m_one_based = one_based; }

    // Node two-sidedness.
    bool two_sided() const { return m_two_sided; }
    void set_two_sided(bool two_sided) { m_two_sided = two_sided; }

    // Get details about the graph.
    std::tuple<int_t, int_t, int_t> get_details() const;

    // Accessors.
    Node& operator[](std::size_t idx) { return m_nodes[idx]; }
    const Node& operator[](std::size_t idx) const { return m_nodes[idx]; }
    node_itr_t begin() { return m_nodes.begin(); }
    node_itr_t end() { return m_nodes.end(); }
    node_const_itr_t begin() const { return m_nodes.begin(); }
    node_const_itr_t end() const { return m_nodes.end(); }

private:
    std::vector<Node> m_nodes;

    bool m_one_based = false;
    bool m_two_sided = false;

};

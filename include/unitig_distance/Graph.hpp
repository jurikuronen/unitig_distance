/*
    A graph data structure.
    Nodes are stored with arbitrary ids, but are accessed and referred using their indices.
    A node doesn't store its index, but its neighbors vector stores the indices of its neighbors in the graph.
    Includes functions for solving connected and biconnected components.
*/

#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Dfs_tree.hpp"
#include "Node.hpp"
#include "types.hpp"

class Graph {
public:
    Graph() { }
    Graph(const Graph& graph) : m_nodes(graph.m_nodes) { }
    /*
        Constructs a graph from nodes and edges text files.
        Nodes file should contain ordered space-separated id and sequence lines.
        Edges file should contain space-separated id1, id2 and edge type lines.
    */
    Graph(const std::string& nodes_filename, const std::string& edges_filename) {
        std::ifstream nodes_file(nodes_filename);
        for (std::string node, sequence; nodes_file >> node >> sequence; ) {
            add_node(std::stoll(node), sequence.size());
        }
        std::ifstream edges_file(edges_filename);
        for (std::string node_1, node_2, edge_type; edges_file >> node_1 >> node_2 >> edge_type; ) {
            add_edge(std::stoll(node_1), std::stoll(node_2)); // Ignore edge type (for now).
        }
        sort_neighbors();
        #ifdef DEBUG
            int_t n_edges = 0, max_degree = 0;
            for (int_t v = 0; v < size(); ++v) {
                int_t sz = m_nodes[v].neighbors().size();
                n_edges += sz;
                max_degree = std::max(max_degree, sz);
            }
            std::cout << "Constructed graph after reading \"" << nodes_filename << "\" and \"" << edges_filename << "\".\n";
            std::cout << "Graph has " << size() << " nodes and " << n_edges / 2 << " edges. Max degree is " << max_degree << ".\n";
        #endif
    }
    /*
        Constructs the subgraph of this graph induced by the nodes given in block.
    */
    Graph(const Graph& graph, const std::vector<int_t>& block) {
        reserve(block.size());
        std::unordered_map<int_t, int_t> node_mapper;
        for (auto i = 0; i < block.size(); ++i) {
            node_mapper[block[i]] = i;
            add_node(block[i]);
        }
        for (auto i = 0; i < block.size(); ++i) {
            for (auto j : graph[block[i]].neighbors()) {
                if (node_mapper.count(j) > 0) add_edge(i, node_mapper[j]);
            }
        }
        sort_neighbors();
    }

    void add_node(int_t id) { m_nodes.emplace_back(id); }
    void add_node(int_t id, int_t weight) { m_nodes.emplace_back(id, weight); }
    void add_edge(std::size_t idx_1, std::size_t idx_2) {
        if (idx_1 == idx_2) return; // Self-edges not allowed.
        m_nodes[idx_1].add_neighbor(idx_2);
        m_nodes[idx_2].add_neighbor(idx_1);
    }
    void remove_edge(std::size_t idx_1, std::size_t idx_2) {
        m_nodes[idx_1].remove_neighbor(idx_2);
        m_nodes[idx_2].remove_neighbor(idx_1);
    }

    std::size_t size() const { return m_nodes.size(); }
    void reserve(std::size_t sz) { m_nodes.reserve(sz); }

    Node& operator[](std::size_t idx) { return m_nodes[idx]; }
    const Node& operator[](std::size_t idx) const { return m_nodes[idx]; }
    typename std::vector<Node>::iterator begin() { return m_nodes.begin(); }
    typename std::vector<Node>::iterator end() { return m_nodes.end(); }
    typename std::vector<Node>::const_iterator begin() const { return m_nodes.begin(); }
    typename std::vector<Node>::const_iterator end() const { return m_nodes.end(); }

    // Finds and marks all connected components in the graph.
    void solve_connected_components();
    // Finds and marks all articulation points in the graph and returns vectors of node ids for all biconnected subgraphs.
    std::vector<std::vector<int_t>> solve_biconnected_components();

private:
    std::vector<Node> m_nodes;

    void sort_neighbors() { for (int_t v = 0; v < size(); ++v) m_nodes[v].sort_neighbors(); }

    // Runs a dfs which marks all nodes in the component. Returns component size for possible debugging.
    int_t connected_component_dfs(std::vector<bool>& visited, int_t v, int_t component_id);

    // Runs a dfs which marks all articulation points in the component. Builds the blocks of a block-cut tree.
    void biconnected_components_dfs(Dfs_tree& dt, std::vector<std::vector<int_t>>& blocks, int_t v);

    // Assembles a block from block_stack.
    std::vector<int_t> get_biconnected_block(std::vector<int_t>& block_stack, int_t v, int_t w);
};

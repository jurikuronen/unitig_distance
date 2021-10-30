#include <algorithm>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Graph.hpp"
#include "Node.hpp"
#include "types.hpp"

// Copy constructor
Graph::Graph(const Graph& graph) = default;

// Construct the combined graph that stores two sides for each node: one for its left side and one for its right side, considered from the canonical form.
Graph::Graph(const std::string& nodes_filename, const std::string& edges_filename, int_t kmer_length) {
    set_two_sided(true); 
    std::ifstream nodes_file(nodes_filename);
    // Add nodes.
    std::string node, sequence;
    nodes_file >> node >> sequence;
    // Check for one-basedness before adding nodes.
    set_one_based(node == "1");
    do {
        real_t self_edge_weight = (real_t) sequence.size() - kmer_length;
        // Add left and right sides.
        add_node();
        add_node();
        add_edge(size() - 2, size() - 1, self_edge_weight);
    } while (nodes_file >> node >> sequence);

    // Add edges.
    std::ifstream edges_file(edges_filename);
    for (std::string node_1, node_2, edge_type; edges_file >> node_1 >> node_2 >> edge_type; ) {
        int_t v = 2 * (std::stoll(node_1) - one_based()) + (edge_type[0] == 'F'); // F* edge means link comes from v's right side.
        int_t w = 2 * (std::stoll(node_2) - one_based()) + (edge_type[1] == 'R'); // *R edge means link goes to w's right side.
        add_edge(v, w, 1.0);
    }
}

// Construct an edge-induced subgraph from the combined graph.
Graph::Graph(const Graph& combined_graph, const std::string& edges_filename) {
    m_nodes.resize(combined_graph.size());
    set_one_based(combined_graph.one_based());
    std::ifstream edges_file(edges_filename);
    for (std::string node_1, node_2, edge_type; edges_file >> node_1 >> node_2 >> edge_type; ) {
        int_t v = 2 * (std::stoll(node_1) - one_based()) + (edge_type[0] == 'F'); // F* edge means link comes from v's right side.
        int_t w = 2 * (std::stoll(node_2) - one_based()) + (edge_type[1] == 'R'); // *R edge means link goes to w's right side.
        // Add self-edges if necessary.
        if ((*this)[v].neighbors().empty()) add_edge(v, v ^ 1, combined_graph[v].get_largest_weight());
        if ((*this)[w].neighbors().empty()) add_edge(w, w ^ 1, combined_graph[w].get_largest_weight());
        // Then add (v, w).
        add_edge(v, w, 1.0);
    }
}

// Construct a filtered subgraph from the combined graph.
Graph::Graph(const Graph& combined_graph, const std::string& filter_filename, int_t criterion) : Graph(combined_graph) {
    // Filter edges based on repeating unitigs.
    std::vector<int_t> values(true_size());
    std::ifstream filter_file(filter_filename);
    for (std::string node_id, value, unitig; filter_file >> node_id >> value >> unitig; ) {
        values[std::stoll(node_id) - one_based()] = std::stoll(value);
    }
    for (std::size_t v = 0; v < true_size(); ++v) {
        if (values[v] >= criterion) {
            remove_neighbors(left_node(v));
            remove_neighbors(right_node(v));
        }
    }
}

void Graph::add_edge(std::size_t idx_1, std::size_t idx_2, real_t weight) {
    if (idx_1 == idx_2) return;
    auto& node_1 = (*this)[idx_1];
    auto& node_2 = (*this)[idx_2];
    auto it_1 = node_1.find_neighbor(idx_2);
    if (it_1 == node_1.end()) {
        // New edge.
        node_1.add_neighbor(idx_2, weight);
        node_2.add_neighbor(idx_1, weight);
    } else {
        // Edge exists, update edge weight.
        auto current_weight = it_1->second;
        if (current_weight <= weight) return;
        it_1->second = weight;
        node_2.find_neighbor(idx_1)->second = weight;
    }
}

void Graph::remove_neighbors(std::size_t idx) {
    auto neighbors = (*this)[idx].neighbors();
    for (auto neighbor : neighbors) remove_edge(idx, neighbor.first);
}

// Remove edge between two nodes.
void Graph::remove_edge(std::size_t idx_1, std::size_t idx_2) {
    (*this)[idx_1].remove_neighbor(idx_2);
    (*this)[idx_2].remove_neighbor(idx_1);
}

std::tuple<int_t, int_t, int_t> Graph::get_details() const {
    int_t n_nodes = 0, n_edges = 0, max_degree = 0;
    for (std::size_t i = 0; i < size(); ++i) {
        int_t sz = (*this)[i].neighbors().size();
        if (two_sided()) {
            sz += (*this)[i + 1].neighbors().size();
            if (sz >= 2) sz -= 2; // Remove "self edge" from these calculations.
            ++i;
        }
        n_nodes += sz > 0;
        n_edges += sz;
        max_degree = std::max(max_degree, sz);
    }
    n_edges /= 2;
    return std::make_tuple(n_nodes, n_edges, max_degree);
}


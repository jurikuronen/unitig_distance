#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Dfs_tree.hpp"
#include "Graph.hpp"
#include "Node.hpp"
#include "types.hpp"

// Finds and marks all connected components in the graph.
void Graph::solve_connected_components() {
    std::vector<bool> visited(size());
    int_t component_id = -1;
    for (int_t v = 0; v < size(); ++v) {
        if (visited[v]) continue;
        connected_component_dfs(visited, v, ++component_id);
    }
}

// Runs a dfs which marks all nodes in the connected component.
void Graph::connected_component_dfs(std::vector<bool>& visited, int_t v, int_t component_id) {
    std::vector<int_t> stack{v};
    while (!stack.empty()) {
        v = stack.back();
        stack.pop_back();
        if (visited[v]) continue;
        visited[v] = true;
        m_nodes[v].set_component_id(component_id);
        for (auto w : m_nodes[v].neighbors()) if (!visited[w]) stack.push_back(w);
    }
}

// Finds and marks all articulation points in the graph and returns vectors of node ids for all biconnected subgraphs.
std::vector<std::vector<int_t>> Graph::solve_biconnected_components() {
    std::vector<std::vector<int_t>> blocks;
    Dfs_tree dt(size());
    for (int_t v = 0; v < size(); ++v) {
        if (dt.dfs(v) >= 0) continue; // Node already processed.
        biconnected_components_dfs(dt, blocks, v);
    }
    return blocks;
}

// Runs a dfs which marks all articulation points in the component. Builds the blocks of a block-cut tree.
void Graph::biconnected_components_dfs(Dfs_tree& dt, std::vector<std::vector<int_t>>& blocks, int_t v) {
    int_t t = 0;
    dt.initialize_root(v);
    std::vector<int_t> stack{v}, block_stack{v};
    while (!stack.empty()) {
        v = stack.back();
        bool visited_all = true;
        for (int_t w : m_nodes[v].neighbors()) {
            if (w == dt.parent(v)) continue;
            auto& vs = dt.visited(v, w);
            if (vs == 0) { // Visiting for the first time
                if (dt.dfs(w) < 0) { // New tree edge.
                    dt.new_tree_edge(v, w, ++t);
                    stack.push_back(w);
                    block_stack.push_back(w);
                    visited_all = false;
                    vs = 1; // Set partially visited.
                    break;
                } else {
                    dt.update_back_edge(v, w);
                }
            } else if (vs == 1) { // Returning from processed tree edge.
                if (dt.low1(w) >= dt.dfs(v)) { 
                    if (dt.not_root_or_first_child_of_root(v, w)) m_nodes[v].set_articulation_point();
                    blocks.push_back(get_biconnected_block(block_stack, v, w));
                }
                dt.update_tree_edge(v, w);
            }
            vs = 2; // Set fully visited.
        }
        if (visited_all) stack.pop_back();
    }
}

// Assembles a block from block_stack.
std::vector<int_t> Graph::get_biconnected_block(std::vector<int_t>& block_stack, int_t v, int_t w) {
    std::vector<int_t> block{v};
    while(block_stack.back() != w) block.push_back(block_stack.back()), block_stack.pop_back();
    block.push_back(block_stack.back()), block_stack.pop_back();
    return block;
}

// Filter edges based on repeating unitigs. Return counts for verbose/debugging.
std::pair<int_t, int_t> Graph::apply_repeating_unitigs_filter(const std::string& repeating_filename, int_t criterion) {
    std::vector<int_t> repeats(size());
    std::ifstream repeating_file(repeating_filename);
    for (std::string node_id, repeating, unitig; repeating_file >> node_id >> repeating >> unitig; ) {
        repeats[std::stoll(node_id)] = std::stoll(repeating);
    }
    int_t removed_nodes = 0, removed_edges = 0;
    for (int_t v = 0; v < size(); ++v) {
        if (repeats[v] < criterion) continue;
        auto neighbors = m_nodes[v].neighbors();
        for (int_t w : neighbors) remove_edge(v, w);
        // Take counts for verbose/debugging.
        ++removed_nodes;
        removed_edges += neighbors.size();
    }
    return {removed_nodes, removed_edges};
}

std::vector<int_t> Graph::read_gerry_filter_data(const std::string& data_line) {
    std::vector<int_t> data;
    std::stringstream ss(data_line);
    for (std::string word; ss >> word; ) {
        data.emplace_back(std::stoll(word));
    }
    return data;
}

// Apply Gerry's filter. Return counts for verbose/debugging.
std::pair<int_t, int_t> Graph::apply_gerry_filter(const std::string& gerry_filename, int_t criterion) {
    std::ifstream gerry_file(gerry_filename);
    std::vector<std::pair<int_t, int_t>> edges_to_remove;
    std::vector<std::pair<int_t, int_t>> edges_to_add;
    m_gerry_edges.resize(size());
    for (std::string line, word; std::getline(gerry_file, line); ) {
        auto data = read_gerry_filter_data(line);
        int_t start_type = data[0];
        int_t end_type = data[1];
        // Criterion 0: Don't filter paths with repeating end points (happens when an end point is at the beginning or end of the assembly path).
        if (criterion == 0 && start_type + end_type != 0) continue;
        int_t new_edge_weight = m_nodes[data[2]].weight();
        for (int_t i = 3; i < data.size(); ++i) {
            int_t node_i = data[i - 1];
            int_t node_j = data[i];
            edges_to_remove.emplace_back(node_i, node_j);
            new_edge_weight += m_nodes[node_j].weight();
        }
        // Criterion 1: Filter paths with repeating end points and add the new edge (including at least one repeated node).
        // Criterion 2: Remove paths with repeating end points without adding a new edge.
        if (criterion == 2 && start_type + end_type != 0) continue;
        edges_to_add.emplace_back(data[2], data.back());
        m_gerry_edges[data[2]][data.back()] += new_edge_weight;
    }
    int_t removed_edges = 0, added_edges = 0;
    for (auto& p : edges_to_remove) {
        int_t node_i, node_j;
        std::tie(node_i, node_j) = p;
        if (m_nodes[node_i].has_neighbor(node_j) && get_gerry_edge_weight(node_i, node_j) == 0) {
            remove_edge(node_i, node_j);
            ++removed_edges;
        }
    }
    for (auto& p : edges_to_add) {
        int_t node_i, node_j;
        std::tie(node_i, node_j) = p;
        if (m_nodes[node_i].has_neighbor(node_j)) continue;
        add_edge(node_i, node_j);
        ++added_edges;
    }
    return {removed_edges, added_edges};
}



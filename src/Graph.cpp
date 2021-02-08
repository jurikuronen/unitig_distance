#include <algorithm>
#include <iostream>
#include <vector>

#include "Dfs_tree.hpp"
#include "Graph.hpp"
#include "Node.hpp"
#include "types.hpp"

/*
    Finds and marks all connected components in the graph.
*/
void Graph::solve_connected_components() {
    std::vector<bool> visited(size());
    int_t component_id = -1;
    #ifdef DEBUG
        std::vector<int_t> component_sizes;
    #endif
    for (int_t v = 0; v < size(); ++v) {
        if (visited[v]) continue;
        auto component_size = connected_component_dfs(visited, v, ++component_id);
        #ifdef DEBUG
            component_sizes.push_back(component_size);
        #endif
    }
    #ifdef DEBUG
        std::sort(component_sizes.rbegin(), component_sizes.rend());
        std::cout << "Found " << component_sizes.size() << " connected components. Top 5 largest:";
        for (auto i = 0; i < std::min<std::size_t>(5, component_sizes.size()); ++i) std::cout << ' ' << component_sizes[i];
        std::cout << ".\n";
    #endif
}

/*
    Runs a dfs which marks all nodes in the connected component.
    Returns component size for possible debugging.
*/
int_t Graph::connected_component_dfs(std::vector<bool>& visited, int_t v, int_t component_id) {
    int_t component_size = 0;
    std::vector<int_t> stack{v};
    while (!stack.empty()) {
        v = stack.back();
        stack.pop_back();
        if (visited[v]) continue;
        visited[v] = true;
        m_nodes[v].set_component_id(component_id);
        ++component_size;
        for (auto w : m_nodes[v].neighbors()) if (!visited[w]) stack.push_back(w);
    }
    return component_size;
}

// Finds and marks all articulation points in the graph and returns vectors of node ids for all biconnected subgraphs.
std::vector<std::vector<int_t>> Graph::solve_biconnected_components() {
    std::vector<std::vector<int_t>> blocks;
    Dfs_tree dt(size());
    for (int_t v = 0; v < size(); ++v) {
        if (dt.dfs(v) >= 0) continue; // Node already processed.
        biconnected_components_dfs(dt, blocks, v);
    }
    #ifdef DEBUG
        int_t n_articulation_points = 0;
        for (int_t v = 0; v < size(); ++v) n_articulation_points += m_nodes[v].is_articulation_point();
        std::cout << "Found " << n_articulation_points << " articulation points.\n";

        std::vector<int_t> block_sizes;
        for (const auto& block : blocks) block_sizes.push_back(block.size());
        std::sort(block_sizes.rbegin(), block_sizes.rend());
        std::cout << "Found " << blocks.size() << " biconnected components. Top 5 largest:";
        for (auto i = 0; i < std::min<std::size_t>(5, block_sizes.size()); ++i) std::cout << ' ' << block_sizes[i];
        std::cout << ".\n";
    #endif
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


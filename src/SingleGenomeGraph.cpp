#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Node.hpp"
#include "SingleGenomeGraph.hpp"
#include "types.hpp"

// Construct a single genome graph, which is a compressed edge-induced subgraph from the combined graph.
SingleGenomeGraph::SingleGenomeGraph(const Graph& combined_graph, const std::string& edges_filename) {
    // Start by constructing the edge-induced subgraph from the combined graph.
    Graph graph(combined_graph, edges_filename);
    m_node_map.resize(graph.size(), std::make_pair(INT_T_MAX, INT_T_MAX));

    std::vector<bool> visited(graph.size());

    // Run a DFS on the edge-induced subgraph to construct a graph with compressed paths.
    for (auto node = 0; node < graph.size(); ++node) {
        if (visited[node] || graph[node].degree() == 0) continue;
        add_and_map_node(node);
        visited[node] = true;
        std::vector<std::tuple<std::size_t, std::size_t, real_t>> stack;
        dfs_add_neighbors_to_stack(graph, visited, stack, node);
        while (!stack.empty()) {
            std::size_t parent, idx;
            real_t weight;
            std::tie(parent, idx, weight) = stack.back();
            stack.pop_back();
            if (visited[idx]) {
                if (!is_on_path(idx)) add_edge(mapped_idx(parent), mapped_idx(idx), weight); // Edge might not be added yet.
                continue;
            }
            if (graph[idx].degree() == 2) process_path(graph, visited, parent, idx, weight); // Compress the path into a single edge, updating idx and weight.
            if (idx == parent) continue; // Path looped back to parent.
            if (mapped_idx(idx) == INT_T_MAX) add_and_map_node(idx); // Check whether path ended at an already mapped node.
            add_edge(mapped_idx(parent), mapped_idx(idx), weight);
            dfs_add_neighbors_to_stack(graph, visited, stack, idx);
            visited[idx] = true;
        }
    }
}

void SingleGenomeGraph::dfs_add_neighbors_to_stack(
    const Graph& graph,
    const std::vector<bool>& visited,
    std::vector<std::tuple<std::size_t, std::size_t, real_t>>& stack,
    std::size_t node)
{
    for (auto neighbor : graph[node]) {
        auto neighbor_idx = neighbor.first;
        auto weight = neighbor.second;
        stack.emplace_back(node, neighbor_idx, weight);
    }
}

// Create a path and compress it into a single edge.
void SingleGenomeGraph::process_path(const Graph& graph, std::vector<bool>& visited, std::size_t path_start_node, std::size_t& idx, real_t& weight) {
    // First node given.
    std::vector<std::size_t> nodes_in_path{idx};
    std::vector<real_t> D{weight};
    auto prev_node = path_start_node;
    while (graph[idx].degree() == 2) {
        auto it = graph[idx].begin();
        if (it->first == prev_node) ++it;
        prev_node = idx;
        std::tie(idx, weight) = *it;
        nodes_in_path.push_back(idx);
        weight += D.back();
        D.push_back(weight);
        if (mapped_idx(idx) != INT_T_MAX) break; // Stop at already mapped node, which is dfs start node or a loop was found.
    }
    auto path_idx = n_paths();
    for (auto i = 0; i + 1 < nodes_in_path.size(); ++i) {
        visited[nodes_in_path[i]] = true;
        map_node(nodes_in_path[i], path_idx, i);
    }
    auto mapped_path_end_node = mapped_idx(idx) != INT_T_MAX ? mapped_idx(idx) : size(); // size() because idx will be added and mapped next.
    add_new_path(mapped_idx(path_start_node), mapped_path_end_node, std::move(D));
}


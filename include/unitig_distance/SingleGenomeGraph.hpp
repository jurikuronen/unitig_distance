#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Graph.hpp"
#include "types.hpp"

class SingleGenomeGraph : public Graph {
public:
    SingleGenomeGraph() { }
    // Construct a single genome graph, which is a compressed edge-induced subgraph from the combined graph.
    SingleGenomeGraph(const Graph& graph, const std::string& edges_filename);

    bool is_on_path(std::size_t original_idx) const { return path_idx(original_idx) != INT_T_MAX; }
    bool contains(std::size_t original_idx) const { return mapped_idx(original_idx) != INT_T_MAX; }

    std::size_t path_idx(std::size_t original_idx) const { return m_node_map[original_idx].first; }
    std::size_t mapped_idx(std::size_t original_idx) const { return m_node_map[original_idx].second; }

    // Path accessors.
    std::size_t start_node(std::size_t path_idx) const { return m_paths[path_idx].start_node; }
    std::size_t end_node(std::size_t path_idx) const { return m_paths[path_idx].end_node; }
    std::pair<std::size_t, real_t> distance_to_start(std::size_t path_idx, std::size_t idx) const {
        return std::make_pair(m_paths[path_idx].start_node, m_paths[path_idx].distance_to_start(idx));
    }
    std::pair<std::size_t, real_t> distance_to_end(std::size_t path_idx, std::size_t idx) const {
        return std::make_pair(m_paths[path_idx].end_node, m_paths[path_idx].distance_to_end(idx));
    }
    real_t distance_in_path(std::size_t path_idx, std::size_t idx_1, std::size_t idx_2) const { return m_paths[path_idx].distance_in_path(idx_1, idx_2); }


private:
    struct Path {
        std::size_t start_node;
        std::size_t end_node;
        std::vector<real_t> DP;

        Path(std::size_t start, std::size_t end, std::vector<real_t>&& D) : start_node(start), end_node(end), DP(std::move(D)) { }

        real_t distance_to_start(std::size_t idx) const { return DP[idx]; }
        real_t distance_to_end(std::size_t idx) const { return DP.back() - DP[idx]; }
        real_t distance_in_path(std::size_t idx_1, std::size_t idx_2) const { return std::abs(DP[idx_1] - DP[idx_2]); }

    };
    std::vector<Path> m_paths;

    std::vector<std::pair<std::size_t, std::size_t>> m_node_map; // Map combined graph indices to this graph.

    // Functions used by the constructor's DFS search.
    void map_node(std::size_t original_idx, std::size_t path_idx, std::size_t mapped_idx) { m_node_map[original_idx] = std::make_pair(path_idx, mapped_idx); }
    void add_and_map_node(std::size_t original_idx) { map_node(original_idx, INT_T_MAX, size()); add_node(); }
    void dfs_add_neighbors_to_stack(
        const Graph& graph,
        const std::vector<bool>& visited,
        std::vector<std::tuple<std::size_t, std::size_t, real_t>>& stack,
        std::size_t node);
    std::size_t n_paths() const { return m_paths.size(); }
    void process_path(const Graph& graph, std::vector<bool>& visited, std::size_t path_start_node, std::size_t& idx, real_t& weight);
    void add_new_path(std::size_t start_node, std::size_t end_node, std::vector<real_t>&& D) { m_paths.emplace_back(start_node, end_node, std::move(D)); }

};


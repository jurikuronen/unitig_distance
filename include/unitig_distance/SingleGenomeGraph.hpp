#pragma once

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Graph.hpp"
#include "types.hpp"

class SingleGenomeGraph : public Graph {
public:
    SingleGenomeGraph() = default;
    ~SingleGenomeGraph() {
        unitig_distance::clear(m_paths);
        unitig_distance::clear(m_node_map);
    }
    SingleGenomeGraph(const SingleGenomeGraph& other) = delete;
    SingleGenomeGraph(SingleGenomeGraph&& other) : Graph(std::move(other)), m_paths(std::move(other.m_paths)), m_node_map(std::move(other.m_node_map)) { }

    // Construct a compressed single genome graph, which is an edge-induced subgraph from the compacted de Bruijn graph.
    SingleGenomeGraph(const Graph& cdbg, const std::string& edges_filename) {
        Graph subgraph(cdbg, edges_filename);

        if (subgraph.size() == 0) return;

        m_node_map.resize(subgraph.size(), std::make_pair(INT_T_MAX, INT_T_MAX));

        std::vector<bool> visited(subgraph.size());

        // Run a DFS on the edge-induced subgraph to construct a graph with compressed paths.
        for (int_t v = 0; v < (int_t) subgraph.size(); ++v) {
            if (visited[v] || subgraph.degree(v) == 0) continue;
            add_and_map_node(v);
            visited[v] = true;
            std::vector<std::tuple<int_t, int_t, real_t>> stack;
            dfs_add_neighbors_to_stack(subgraph, visited, stack, v);
            while (!stack.empty()) {
                int_t parent, w;
                real_t weight;
                std::tie(parent, w, weight) = stack.back();
                stack.pop_back();
                if (visited[w]) {
                    if (!is_on_path(w)) add_edge(mapped_idx(parent), mapped_idx(w), weight); // Edge might not be added yet.
                    continue;
                }
                if (subgraph.degree(w) == 2) {
                    // Compress the path into a single edge, updating w and weight.
                    std::tie(w, weight) = process_path(subgraph, visited, parent, w, weight);
                    if (w == parent) continue; // Path looped back to parent.
                }
                if (!is_mapped(w)) add_and_map_node(w);
                add_edge(mapped_idx(parent), mapped_idx(w), weight);
                dfs_add_neighbors_to_stack(subgraph, visited, stack, w);
                visited[w] = true;
            }
        }
    }

    bool is_on_path(int_t original_idx) const { return path_idx(original_idx) != INT_T_MAX; }

    bool contains(int_t original_idx) const { return original_idx < (int_t) m_node_map.size() && is_mapped(original_idx); }

    bool contains_original(int_t v) const { return contains(left_node(v)); }

    int_t path_idx(int_t original_idx) const { return m_node_map[original_idx].first; }

    int_t mapped_idx(int_t original_idx) const { return m_node_map[original_idx].second; }

    bool is_mapped(int_t original_idx) const { return mapped_idx(original_idx) != INT_T_MAX; }

    // Path accessors.
    int_t start_node(int_t path_idx) const { return m_paths[path_idx].start_node; }

    int_t end_node(int_t path_idx) const { return m_paths[path_idx].end_node; }

    std::pair<int_t, real_t> distance_to_start(int_t path_idx, int_t idx) const {
        const auto& path = m_paths[path_idx];
        return std::make_pair(path.start_node, path.distance_to_start(idx));
    }

    std::pair<int_t, real_t> distance_to_end(int_t path_idx, int_t idx) const {
        const auto& path = m_paths[path_idx];
        return std::make_pair(path.end_node, path.distance_to_end(idx));
    }

    real_t distance_in_path(int_t path_idx, int_t idx_1, int_t idx_2) const { return m_paths[path_idx].distance_in_path(idx_1, idx_2); }

    void swap(SingleGenomeGraph& other) {
        SingleGenomeGraph tmp = std::move(*this);
        *this = std::move(other);
        other = std::move(tmp);
    }

    SingleGenomeGraph& operator=(const SingleGenomeGraph& other) = delete;
    SingleGenomeGraph& operator=(SingleGenomeGraph&& other) {
        Graph::operator=(std::move(other));
        m_paths = std::move(other.m_paths);
        m_node_map = std::move(other.m_node_map);
        return *this;
    }

private:
    struct Path {
        int_t start_node;
        int_t end_node;
        std::vector<real_t> DP;

        Path(int_t start, int_t end, std::vector<real_t>&& D) : start_node(start), end_node(end), DP(std::move(D)) { }

        real_t distance_to_start(int_t idx) const { return DP[idx]; }
        real_t distance_to_end(int_t idx) const { return DP.back() - DP[idx]; }
        real_t distance_in_path(int_t idx_1, int_t idx_2) const { return std::abs(DP[idx_1] - DP[idx_2]); }

    };
    std::vector<Path> m_paths;

    std::vector<std::pair<int_t, int_t>> m_node_map; // Map original graph indices to this graph as (path_idx, mapped_idx) pairs.

    // Functions used by the constructor's DFS search.
    void map_node(int_t original_idx, int_t path_idx, int_t mapped_idx) { m_node_map[original_idx] = std::make_pair(path_idx, mapped_idx); }

    void add_and_map_node(int_t original_idx) { map_node(original_idx, INT_T_MAX, size()); add_node(); } // Add non-path node.

    void dfs_add_neighbors_to_stack(
        const Graph& subgraph,
        const std::vector<bool>& visited,
        std::vector<std::tuple<int_t, int_t, real_t>>& stack,
        int_t original_idx)
    {
        for (auto neighbor : subgraph[original_idx]) {
            auto neighbor_idx = neighbor.first;
            auto weight = neighbor.second;
            stack.emplace_back(original_idx, neighbor_idx, weight);
        }
    }

    std::pair<int_t, real_t> process_path(const Graph& subgraph, std::vector<bool>& visited, int_t path_start_node, int_t w, real_t weight) {
        // First node given.
        std::vector<int_t> nodes_in_path{w};
        std::vector<real_t> D{weight};
        auto prev_node = path_start_node;

        // Move along the path.
        while (subgraph.degree(w) == 2) {
            auto it = subgraph[w].begin();
            if (it->first == prev_node) ++it;
            prev_node = w;
            std::tie(w, weight) = *it;
            nodes_in_path.push_back(w);
            weight += D.back(); // Accumulate weight.
            D.push_back(weight);
            if (is_mapped(w)) break; // Reached end of path (or a loop was found).
        }

        // Map nodes in path as path nodes. Path start and end nodes treated separately.
        auto new_path_idx = m_paths.size();
        for (std::size_t i = 0; i + 1 < nodes_in_path.size(); ++i) {
            visited[nodes_in_path[i]] = true;
            map_node(nodes_in_path[i], new_path_idx, i);
        }

        // Add new path.
        auto mapped_path_end_node = is_mapped(w) ? mapped_idx(w) : size(); // Can use size() here because w will be added and mapped next.
        add_new_path(mapped_idx(path_start_node), mapped_path_end_node, std::move(D));
        return std::make_pair(w, weight); // Return updated w and weight.
    }

    void add_new_path(int_t start_node, int_t end_node, std::vector<real_t>&& D) { m_paths.emplace_back(start_node, end_node, std::move(D)); }

};


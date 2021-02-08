/*
    Implements the 3-vertex-connectivity algorithm by Abusayeed M Saifullah and Alper Üngör (2009).
*/

#include <iostream>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "Dfs_tree.hpp"
#include "Graph.hpp"
#include "graph_tcc.hpp"
#include "graph_tcc_reduce.hpp"
#include "graph_tecc.hpp"
#include "Timer.hpp"
#include "types.hpp"

// Finds most cut vertices in the (sub)graph and returns a vector of their ids in the original graph.
std::vector<int_t> solve_cut_vertices(const Graph& graph) {
    std::vector<int_t> cut_vertices;
    std::set<int_t> cut_vertices_set;
    auto dt = triconnected_components_dfs(graph); // Construct dfs tree with a preliminary DFS.
    check_type_a_cut_pairs(dt, cut_vertices_set); // Check case type A cut pairs.
    auto reduced_graph = construct_reduced_graph(graph, dt); // graph_tcc_reduce.hpp; dt matches reduced_graph after.
    auto cut_edge_pairs = solve_cut_edge_pairs(reduced_graph); // graph_tecc.hpp.
    check_type_b_cut_pairs(reduced_graph, dt, cut_edge_pairs, cut_vertices_set); // Check remaining type B cut pairs.
    for (auto cv : cut_vertices_set) cut_vertices.push_back(graph[cv].id());
    return cut_vertices;
}

// Runs preliminary DFS on graph.
Dfs_tree triconnected_components_dfs(const Graph& graph) {
    Dfs_tree dt(graph.size());
    int_t t = 0, v = 0;
    dt.initialize_root(v);
    std::vector<int_t> stack{v};
    while (!stack.empty()) {
        v = stack.back();
        bool visited_all = true;
        for (int_t w : graph[v].neighbors()) {
            if (w == dt.parent(v)) continue;
            auto& vs = dt.visited(v, w);
            if (vs == 0) { // Visiting for the first time
                if (dt.dfs(w) < 0) { // New tree edge.
                    dt.new_tree_edge(v, w, ++t);
                    stack.push_back(w);
                    visited_all = false;
                    vs = 1; // Set partially visited.
                    break;
                } else if (dt.dfs(w) < dt.dfs(v)) dt.update_back_edge(v, w); // Outgoing back edge.
            } else if (vs == 1) dt.update_tree_edge(v, w); // Returning from processed tree edge.
            vs = 2; // Set fully visited.
        }
        if (visited_all) stack.pop_back();
    }
    return dt;
}

// Check the simple type A cut pairs.
// {x, y} is a type A cut pair if V3, and V1 or V2 are non-empty and there is no back edge from V3 to V1 or V2.
// Refer to the paper for additional details.
void check_type_a_cut_pairs(Dfs_tree& dt, std::set<int_t>& cut_vertices_set) {
    for (auto v = 0; v < dt.size(); ++v) {
        auto y = dt.parent(v), x = dt.dfs_map(dt.low1(v));
        if (!dt.is_null(y) && x != y) {
            bool V1 = !dt.is_root(x), V2 = x != dt.parent(y); // |V1| > 0 or |V2| > 0
            if ((V1 || V2) && dt.low2(v) >= dt.dfs(y)) { // || |V3| > 0 because we are checking with v.
                cut_vertices_set.insert(x);
                cut_vertices_set.insert(y);
            }
        }
    }
}

// Check type B cut pairs after the 3-edge-connectivity algorithm was run on the reduced graph.
// Let {(v0, w0), (v1, w1)} be the cut edge pair found by the 3-edge-connectivity algorithm, x = {v0, w0} and y = {v1, w1}.
// {x, y} is a type B cut pair if (v0, w0) and (v1, w1) are tree edges in the reduced graph, {x, y} is not a root leaf pair and V2 is non-empty.
// Refer to the paper for additional details.
void check_type_b_cut_pairs(const Graph& reduced_graph, Dfs_tree& dt, const std::vector<std::tuple<int_t, int_t, int_t, int_t>>& cut_edge_pairs, std::set<int_t>& cut_vertices_set) {
    int_t v0, w0, v1, w1, x, y, xo, yo;
    for (auto t : cut_edge_pairs) {
        std::tie(v0, w0, v1, w1) = t;
        if (!dt.is_tree_edge(v0, w0) || !dt.is_tree_edge(v1, w1)) continue; // Must be tree edges in reduced graph dfs tree.
        std::vector<std::pair<int_t, int_t>> cases{{v0, v1}, {v0, w1}, {w0, v1}, {w0, w1}};
        for (auto c : cases) {
            std::tie(x, y) = c;
            std::tie(xo, yo) = std::make_pair(reduced_graph[x].id(), reduced_graph[y].id()); // Ids in original graph.
            if (xo == yo) continue;
            if (dt.is_root_leaf_pair(xo, yo)) continue; // Query in dt still valid for original graph.
            if (V2_empty(reduced_graph, x, y)) continue;
            cut_vertices_set.insert(xo);
            cut_vertices_set.insert(yo);
        }
    }
}

// Rather arbitrary checks for the |V2| != 0 case:
// - Check whether e1 and e2 are neighbors on a path, then V2 is empty. 
// - Check whether e1 and e2 are on a path and cut only reduced nodes that are either x or y.
bool V2_empty(const Graph& reduced_graph, int_t x, int_t y) {
    int_t xo = reduced_graph[x].id(), yo = reduced_graph[y].id();
    // Check whether e1 and e2 are neighbors on a path.
    if (reduced_graph[x].neighbors().size() == 2 && reduced_graph[x].has_neighbor(y)) return true;
    if (reduced_graph[y].neighbors().size() == 2 && reduced_graph[y].has_neighbor(x)) return true;
    for (int_t v : reduced_graph[x].neighbors()) {
        if (reduced_graph[v].neighbors().size() == 2) {
            // Check if y is along this path.
            int_t w = v, prev = x;
            bool found_real_nodes = false;
            // Move along the path until the path ends or we come across y.
            while (reduced_graph[w].neighbors().size() == 2 && w != y) {
                int_t wo = reduced_graph[w].id();
                if (wo != xo && wo != yo) found_real_nodes = true;
                auto& ne = reduced_graph[w].neighbors();
                bool b = ne[0] == prev;
                prev = w;
                w = b ? ne[1] : ne[0];
            }
            // If y was along this path, check whether the path contained any real nodes.
            if (!found_real_nodes && w == y) return true;
        }
    }
    return false;
}


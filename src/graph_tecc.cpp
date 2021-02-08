#include <map>
#include <tuple>
#include <vector>

#include "Dfs_tree.hpp"
#include "Graph.hpp"
#include "graph_tecc.hpp"
#include "graph_tecc_stack.hpp"
#include "types.hpp"

// Main dfs for the 3-edge-connectivity algorithm by Yung H. Tsin (2009).
std::vector<std::tuple<int_t, int_t, int_t, int_t>> solve_cut_edge_pairs(const Graph& graph) {
    std::vector<std::tuple<int_t, int_t, int_t, int_t>> cut_edge_pairs;
    int_t t = 0, x, y, p, q;
    Dfs_tree dt(graph.size());
    dt.initialize_root(0);
    std::vector<int_t> stack{0};
    std::vector<tecc_stack> edge_stack(graph.size());
    while (!stack.empty()) {
        auto v = stack.back();
        bool visited_all = true;
        for (auto w : graph[v].neighbors()) {
            if (w == dt.parent(v)) continue;
            auto& vs = dt.visited(v, w);
            if (vs == 0) { // Visiting for the first time
                if (dt.dfs(w) < 0) { // Tree edge.
                    dt.new_tree_edge(v, w, ++t);
                    stack.push_back(w);
                    visited_all = false;
                    vs = 1; // Set partially visited.
                    break;
                } else if (dt.dfs(w) < dt.dfs(v)) { // (v, w) outgoing back edge of v
                    if (dt.dfs(w) <= dt.low1(v)) edge_stack[v].clear(); 
                    dt.update_back_edge_tecc(v, w);
                }
            } else if (vs == 1) { // Returning from processed tree edge.
                if (!edge_stack[w].empty() && w == edge_stack[w].top_q()) {
                    std::tie(x, y, p, q) = edge_stack[w].pop();
                    cut_edge_pairs.emplace_back(x, y, v, w); // Cut edge pair {(x, y), (v, w)} found.
                    if (v != p) edge_stack[w].push(x, y, p, v);
                }
                if (dt.low1(w) < dt.low1(v)) edge_stack[v] = edge_stack[w];
                else if (dt.low1(w) < dt.low2(v)) edge_stack[w].clear();
                dt.update_tree_edge_tecc(v, w); // Important order of functions.
            }
            vs = 2; // Set fully visited.
        }
        if (visited_all) {
            update_edge_stack(dt, edge_stack[v], v);
            stack.pop_back();
        }
    }
    return cut_edge_pairs;
}

// Update v's edge_stack.
void update_edge_stack(Dfs_tree& dt, tecc_stack& edge_stack, int_t v) {
    if (edge_stack.empty()) {
        if (dt.low2(v) > dt.low1(v)) edge_stack.push(v, dt.to_low(v), dt.dfs_map(dt.low1(v)), dt.dfs_map(dt.low2(v)));
    } else if (dt.low2(v) > dt.dfs(edge_stack.top_q())) {
        edge_stack.push(v, dt.to_low(v), edge_stack.top_q(), dt.dfs_map(dt.low2(v)));
    } else {
        while (!edge_stack.empty() && dt.low2(v) <= dt.dfs(edge_stack.top_p())) edge_stack.pop();
        if (!edge_stack.empty() && dt.low2(v) < dt.dfs(edge_stack.top_q())) edge_stack.top_q() = dt.dfs_map(dt.low2(v));
    }
    for (auto u : dt.in(v)) {
        if (edge_stack.empty()) break;
        while (!edge_stack.empty()) {
            auto x = edge_stack.top_x(), y = edge_stack.top_y();
            if (!dt.is_tree_edge(x, y) || !dt.is_ancestor(y, u)) break;
            edge_stack.pop();
        }
    }
}


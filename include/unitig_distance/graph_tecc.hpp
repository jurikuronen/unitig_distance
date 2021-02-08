/*
    Implements the the 3-edge-connectivity algorithm by Yung H. Tsin (2009).
*/

#pragma once

#include <tuple>
#include <vector>

#include "Dfs_tree.hpp"
#include "Graph.hpp"
#include "graph_tecc_stack.hpp"
#include "types.hpp"

// Main dfs for the 3-edge-connectivity algorithm by Yung H. Tsin (2009).
std::vector<std::tuple<int_t, int_t, int_t, int_t>> solve_cut_edge_pairs(const Graph& graph);
// Update v's edge_stack.
void update_edge_stack(Dfs_tree& dt, tecc_stack& edge_stack, int_t v);

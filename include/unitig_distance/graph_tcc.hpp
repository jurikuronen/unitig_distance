/*
    Implements the 3-vertex-connectivity algorithm by Abusayeed M Saifullah and Alper Üngör (2009).
*/

#pragma once

#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "Dfs_tree.hpp"
#include "Graph.hpp"
#include "types.hpp"

std::vector<int_t> solve_cut_vertices(const Graph& graph);

// Runs preliminary DFS on graph.
Dfs_tree triconnected_components_dfs(const Graph& graph);

// Check the simple type A cut pairs.
// {x, y} is a type A cut pair if V3, and V1 or V2 are non-empty and there is no back edge from V3 to V1 or V2.
// Refer to the paper for additional details.
void check_type_a_cut_pairs(Dfs_tree& dt, std::set<int_t>& cut_vertices_set);

// Check type B cut pairs after the 3-edge-connectivity algorithm was run on the reduced graph.
// Let {(v0, w0), (v1, w1)} be the cut edge pair found by the 3-edge-connectivity algorithm, x = {v0, w0} and y = {v1, w1}.
// {x, y} is a type B cut pair if (v0, w0) and (v1, w1) are tree edges in the reduced graph, {x, y} is not a root leaf pair and V2 is non-empty.
// Refer to the paper for additional details.
void check_type_b_cut_pairs(const Graph& reduced_graph, Dfs_tree& dt, const std::vector<std::tuple<int_t, int_t, int_t, int_t>>& cut_edge_pairs, std::set<int_t>& cut_vertices_set);

// Rather arbitrary checks for the |V2| != 0 case:
// - Check whether e1 and e2 are neighbors on a path, then V2 is empty. 
// - Check whether e1 and e2 are on a path and cut only reduced nodes that are either x or y.
bool V2_empty(const Graph& reduced_graph, int_t x, int_t y);

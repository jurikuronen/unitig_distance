/*
    Implements the reduce operation for the 3-vertex-connectivity algorithm by Abusayeed M Saifullah and Alper Üngör (2009).
*/

#pragma once

#include <vector>

#include "Dfs_tree.hpp"
#include "Graph.hpp"
#include "types.hpp"

// Construct the reduced graph for determining type B separation pairs.
Graph construct_reduced_graph(const Graph& graph, Dfs_tree& dt);

// Change edge (v, w) into (v_new, w) in the graph.
inline void replace_edge(Graph& reduced_graph, int_t v, int_t w, int_t v_new);

// Replace node in some container with node_new.
inline void replace_node(std::vector<int_t>& vector, int_t node, int_t node_new);

// Auxiliary function for adding a new tree edge (v, w).
inline void add_tree_edge(Graph& reduced_graph, Dfs_tree& dt, int_t v, int_t w);

// If low1[v] comes from outgoing back edges of v only, create a fictitious child of v.
bool fictitious_child_condition(const Graph& graph, Dfs_tree& dt, int_t v);

// Create a fictitious child of v which becomes a leaf node with all outgoing back edges of v.
// This simplifies checking type B separation pairs.
int_t create_fictitious_child(Graph& reduced_graph, Dfs_tree& dt, int_t v);

// Reduce v into a tree path v_outk - ... - v_out1 - v_inl - ... - vin1, sorted by out/in nodes' dfs (v_outk / v_inl largest).
// v becomes the first node (so that the for-loop in construct_reduced_graph() works), create other nodes.
// Then relink v's outcoming/incoming back edges to these nodes.
// Called when |out(v)| + |in(v)| > 1.
void reduce(Graph& reduced_graph, Dfs_tree& dt, int_t v);

// Sort v's outgoing and incoming back edges with respect to dfs values.
void sort_out_and_in(Dfs_tree& dt, int_t v);


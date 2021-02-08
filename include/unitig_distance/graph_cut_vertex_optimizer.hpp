/*
    Remove redundant cut vertices.
*/
#pragma once

#include "Graph.hpp"
#include "types.hpp"

// Very initial versions of this operation.

bool node_is_on_path(Graph& graph, int_t v) { return graph[v].neighbors().size() == 2; }

void clear_path(Graph& graph, int_t v) {
    graph[v].unset_cut_node();
    for (int_t w : graph[v].neighbors()) {
        if (graph[w].is_cut_node() && node_is_on_path(graph, w)) clear_path(graph, w);
    }
}

void optimize_cut_vertices_along_paths(Graph& graph) {
    for (int_t v = 0; v < graph.size(); ++v) {
        if (!graph[v].is_cut_node()) continue;
        if (node_is_on_path(graph, v)) clear_path(graph, v);
    }
}

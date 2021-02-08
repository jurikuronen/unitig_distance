#include <algorithm>
#include <iostream>
#include <vector>

#include "Dfs_tree.hpp"
#include "Graph.hpp"
#include "graph_tcc_reduce.hpp"
#include "types.hpp"

// Construct the reduced graph for determining type B separation pairs.
Graph construct_reduced_graph(const Graph& graph, Dfs_tree& dt) {
    auto reduced_graph = graph; // Create a copy of the graph.
    // Reset the unnecessary ids, we need to use them to store information about reduced nodes.
    for (int_t i = 0; i < reduced_graph.size(); ++i) reduced_graph[i].set_id(i);
    for (int_t i = dt.size() - 1; i >= 0; --i) { // Start from last visited node.
        auto v = dt.dfs_map(i);
        if (fictitious_child_condition(graph, dt, v)) { // low1[v] comes from outgoing back edges of v only.
            auto fc = create_fictitious_child(reduced_graph, dt, v);
            if (dt.out(fc).size() > 1) reduce(reduced_graph, dt, fc); // Reduce also fc; in should be empty.
        }
        // Reduce v when incident to least 2 back edges.
        if (dt.out(v).size() + dt.in(v).size() > 1) reduce(reduced_graph, dt, v);
    }
    return reduced_graph;
}

// Change edge (v, w) into (v_new, w) in the graph.
inline void replace_edge(Graph& reduced_graph, int_t v, int_t w, int_t v_new) {
    reduced_graph.remove_edge(v, w);
    reduced_graph.add_edge(v_new, w);
}

// Replace node in some container with node_new.
inline void replace_node(std::vector<int_t>& vector, int_t node, int_t node_new) { *std::find(vector.begin(), vector.end(), node) = node_new; }

// Auxiliary function for adding a new tree edge (v, w).
inline void add_tree_edge(Graph& reduced_graph, Dfs_tree& dt, int_t v, int_t w) {
    reduced_graph.add_edge(v, w);
    dt.parent(w) = v;
}

// If low1[v] comes from outgoing back edges of v only, create a fictitious child of v.
bool fictitious_child_condition(const Graph& graph, Dfs_tree& dt, int_t v) {
    if (dt.is_leaf(v)) return false;
    for (auto w : graph[v].neighbors()) {
        // Skip parent, back edge or tree edge with low1[w] > low1[v].
        if (w == dt.parent(v) || dt.parent(w) != v || dt.low1(v) != dt.low1(w)) continue;
        return false;
    }
    return true;
}

// Create a fictitious child of v which becomes a leaf node with all outgoing back edges of v.
// This simplifies checking type B separation pairs.
int_t create_fictitious_child(Graph& reduced_graph, Dfs_tree& dt, int_t v) {
    int_t fc = reduced_graph.size();
    dt.add_node(v);
    reduced_graph.add_node(reduced_graph[v].id());
    add_tree_edge(reduced_graph, dt, v, fc);
    // Move outgoing back edges.
    dt.out(fc) = dt.out(v);
    for (auto w : dt.out(fc)) {
        replace_edge(reduced_graph, v, w, fc);
        replace_node(dt.in(w), v, fc);
    }
    dt.out(v).clear();
    return fc;
}

// Reduce v into a tree path v_outk - ... - v_out1 - v_inl - ... - vin1, sorted by out/in nodes' dfs (v_outk / v_inl largest).
// v becomes the first node (so that the for-loop in construct_reduced_graph() works), create other nodes.
// Then relink v's outcoming/incoming back edges to these nodes.
// Called when |out(v)| + |in(v)| > 1.
void reduce(Graph& reduced_graph, Dfs_tree& dt, int_t v) {
    sort_out_and_in(dt, v);
    int_t v_id = reduced_graph[v].id();

    // Store v's original children.
    std::vector<int_t> v_children;
    for (auto w : reduced_graph[v].neighbors()) if (dt.parent(w) == v) v_children.push_back(w);

    // First create the tree path before linking out/in nodes.
    std::vector<int_t> v_out, v_in;
    // Booleans to determine whether v becomes v_out.back() or v_in.back().
    bool out_empty = dt.out(v).size() == 0;
    bool in_empty = dt.in(v).size() == 0;
    // Tree path for in nodes.
    for (int_t i = 0; i < (int_t) dt.in(v).size() - out_empty; ++i) { // v becomes v_in.back() if out is empty.
        dt.add_node(v);
        v_in.push_back(reduced_graph.size());
        reduced_graph.add_node(v_id);
        if (i > 0) add_tree_edge(reduced_graph, dt, v_in[i], v_in[i - 1]);
    }
    // Tree path for out nodes.
    for (int_t i = 0; i < (int_t) dt.out(v).size() - 1; ++i) { 
        dt.add_node(v);
        v_out.push_back(reduced_graph.size());
        reduced_graph.add_node(v_id);
        if (i == 0 && !in_empty) add_tree_edge(reduced_graph, dt, v_out[i], v_in.back());
        else if (i > 0) add_tree_edge(reduced_graph, dt, v_out[i], v_out[i - 1]);
    }

    // Add v as one of the nodes in the path.
    if (out_empty) { // in must contain >= 2 nodes.
        add_tree_edge(reduced_graph, dt, v, v_in.back());
        v_in.push_back(v);
    } else {
        if (v_out.size() > 0) add_tree_edge(reduced_graph, dt, v, v_out.back());
        else add_tree_edge(reduced_graph, dt, v, v_in.back()); // in contains >= 1 nodes.
        v_out.push_back(v);
    }
    // Move v's children to the front of the tree path.
    int_t front_node = !in_empty ? v_in.front() : v_out.front();
    for (auto w : v_children) {
        replace_edge(reduced_graph, v, w, front_node);
        dt.parent(w) = front_node;
    }

    // Link v's out/in nodes to the newly created tree path.
    for (int_t i = 0; i < (int_t) v_in.size() - out_empty; ++i) {
        auto vii = dt.in(v)[i];
        dt.in(v_in[i]).push_back(vii);
        replace_node(dt.out(vii), v, v_in[i]);
        replace_edge(reduced_graph, v, vii, v_in[i]);
    }
    for (int_t i = 0; i < (int_t) v_out.size() - 1; ++i) { // v is either v_out.back() or v_in.back().
        auto voi = dt.out(v)[i];
        dt.out(v_out[i]).push_back(voi);
        replace_node(dt.in(voi), v, v_out[i]);
        replace_edge(reduced_graph, v, voi, v_out[i]);
    }
    // Clear v's (part of the tree path now) in and out nodes.
    dt.in(v).clear();
    if (out_empty) {
        dt.in(v).push_back(v_in.back());
    } else {
        dt.out(v).clear();
        dt.out(v).push_back(v_out.back());
    }
}

// Sort v's outgoing and incoming back edges with respect to dfs values.
void sort_out_and_in(Dfs_tree& dt, int_t v) {
    auto sort_by_dfs = [&dt](int_t v, int_t w) { return dt.dfs(v) < dt.dfs(w); };
    std::sort(dt.out(v).begin(), dt.out(v).end(), sort_by_dfs);
    std::sort(dt.in(v).begin(), dt.in(v).end(), sort_by_dfs);
}


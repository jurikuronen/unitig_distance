/*
    Tree data structure which stores information encoding, and relevant to, a DFS tree.
    Provides auxiliary functions to aid the search.
    The search must be implemented by the user.
*/

#pragma once

#include <utility>
#include <vector>

#include "types.hpp"

class Dfs_tree {
public:
    Dfs_tree(int_t sz) :
        m_dfs(std::vector<int_t>(sz, -1)),
        m_dfs_map(std::vector<int_t>(sz)),
        m_low1(std::vector<int_t>(sz)),
        m_low2(std::vector<int_t>(sz)),
        m_to_low(std::vector<int_t>(sz)),
        m_parent(std::vector<int_t>(sz, -1)),
        m_nd(std::vector<int_t>(sz, 1)),
        m_in(std::vector<std::vector<int_t>>(sz)),
        m_out(std::vector<std::vector<int_t>>(sz)),
        m_visited(std::vector<std::vector<std::pair<int_t, int_t>>>(sz)) { }

    std::size_t size() { return m_dfs.size(); }

    // Accessors.
    int_t& dfs(int_t idx) { return m_dfs[idx]; }
    int_t& dfs_map(int_t idx) { return m_dfs_map[idx]; }
    int_t& low1(int_t idx) { return m_low1[idx]; }
    int_t& low2(int_t idx) { return m_low2[idx]; }
    int_t& to_low(int_t idx) { return m_to_low[idx]; }
    int_t& parent(int_t idx) { return m_parent[idx]; }
    int_t& nd(int_t idx) { return m_nd[idx]; }
    std::vector<int_t>& in(int_t idx) { return m_in[idx]; }
    std::vector<int_t>& out(int_t idx) { return m_out[idx]; }
    int_t& visited(int_t v, int_t w) {
        for (auto& p : m_visited[v]) if (p.first == w) return p.second;
        m_visited[v].emplace_back(w, 0); // Add (v, w) zero-initialized if not found.
        return m_visited[v].back().second;
    }

    // Queries about vertices v and w.
    bool is_null(int_t v) { return v == -1; }
    bool is_root(int_t v) { return dfs(v) == 0; }
    bool is_leaf(int_t v) { return nd(v) == 1; }
    bool is_root_leaf_pair(int_t v, int_t w) { return (is_root(v) && is_leaf(w)) || (is_root(w) && is_leaf(v)); }
    bool is_tree_edge(int_t v, int_t w) { return v == parent(w) || w == parent(v); }
    bool is_ancestor(int_t v, int_t w) { return dfs(v) <= dfs(w) && dfs(w) <= dfs(v) + nd(v) - 1; }
    bool not_root_or_first_child_of_root(int_t v, int_t w) { return !is_root(v) || dfs(w) > 1; }

    // Functions used by the search.
    void initialize_root(int_t root) { dfs(root) = low1(root) = low2(root) = dfs_map(root) = 0; }
    void new_tree_edge(int_t v, int_t w, int_t t) {
        parent(w) = v;
        dfs(w) = low1(w) = low2(w) = t;
        dfs_map(t) = w;
    }
    void new_back_edge(int_t v, int_t w) { out(v).push_back(w); in(w).push_back(v); }

    // Following functions update low points after encountering back edges or returning from processed tree edges.
    void update_back_edge(int_t v, int_t w) {
        new_back_edge(v, w);
        if (dfs(w) < low1(v)) {
            low2(v) = low1(v);
            low1(v) = dfs(w);
        } else if (dfs(w) > low1(v)) {
            low2(v) = std::min(low2(v), dfs(w));
        }
    }
    void update_tree_edge(int_t v, int_t w) {
        if (low1(w) < low1(v)) {
            low2(v) = std::min(low1(v), low2(w));
            low1(v) = low1(w);
        } else if (low1(w) == low1(v)) {
            low2(v) = std::min(low2(v), low2(w));
        } else {
            low2(v) = std::min(low2(v), low1(w));
        }
        nd(v) += nd(w);
    }

    // Definition of low2 is different for the 3-edge-connectivity algorithm: e.g. low1(v) = low2(v) is allowed as long as the paths are edge-disjoint.
    // Updates also to_low, which is only used by the 3-edge-connectivity algorithm.
    void update_back_edge_tecc(int_t v, int_t w) {
        new_back_edge(v, w);
        if (dfs(w) <= low1(v)) {
            low2(v) = low1(v);
            low1(v) = dfs(w);
            to_low(v) = w;
        } else if (dfs(w) < low2(v)) {
            low2(v) = dfs(w);
        }
    }
    void update_tree_edge_tecc(int_t v, int_t w) {
        if (low1(w) < low1(v)) {
            low2(v) = low1(v);
            low1(v) = low1(w);
            to_low(v) = w;
        } else if (low1(w) < low2(v)) {
            low2(v) = low1(w);
        }
        nd(v) += nd(w);
    }

    // Special function used by graph_tcc_reduce.cpp.
    void add_node(int_t v) {
        m_dfs.emplace_back(dfs(v));
        m_parent.emplace_back(-1);
        m_in.emplace_back();
        m_out.emplace_back();
    }

private:
    std::vector<int_t> m_dfs;
    std::vector<int_t> m_dfs_map;
    std::vector<int_t> m_low1;
    std::vector<int_t> m_low2;
    std::vector<int_t> m_to_low; // Back edge (v, w) s.t. dfs(w) = low1(v) or first child w of v encountered during the search s.t. low1(w) = low1(v)
    std::vector<int_t> m_parent;
    std::vector<int_t> m_nd;
    std::vector<std::vector<int_t>> m_in;
    std::vector<std::vector<int_t>> m_out;
    std::vector<std::vector<std::pair<int_t, int_t>>> m_visited;

};

/*
    Various diagnostics that provide information about a graph.
*/

#pragma once

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <set>
#include <vector>

#include "Graph.hpp"
#include "types.hpp"

void vertex_degrees(const Graph& graph) {
    std::vector<int_t> degrees(graph.size());
    for (int_t v = 0; v < graph.size(); ++v) degrees[v] = graph[v].neighbors().size();
    std::sort(degrees.begin(), degrees.end());
    int_t max_degree = degrees.back();
    std::vector<int_t> degrees_count(max_degree + 1);
    for (int_t i = 0; i < graph.size(); ++i) ++degrees_count[degrees[i]];
    for (int_t degree = 0; degree <= max_degree; ++degree) {
        std::printf("graph_diagnostics  ::  Vertices with degree %2d: %d\n", (int) degree, (int) degrees_count[degree]);
    };
    std::vector<double> quantiles{0.1, 0.25, 0.5, 0.75, 0.9, 0.95, 0.99, 0.999, 1.0};
    std::set<int_t> degrees_seen;
    for (double q : quantiles) {
        int_t idx = q * graph.size() - 1;
        int_t degree = degrees[idx];
        if (degrees_seen.count(degree)) continue;
        degrees_seen.insert(degree);
        idx = std::distance(degrees.begin(), std::find(degrees.begin(), degrees.end(), degree));
        q = 1.0 * idx / graph.size();
        std::printf("graph_diagnostics  ::  Quantile %.3f degree: %7d (%d nodes with >= this degree)\n", q, (int) degrees[idx], (int) (graph.size() - idx));
    }
}

void weight_distribution(const Graph& graph) {
    std::vector<int_t> weights(graph.size());
    for (int_t v = 0; v < graph.size(); ++v) weights[v] = graph[v].weight();
    std::sort(weights.begin(), weights.end());
    std::vector<double> quantiles{0.1, 0.25, 0.5, 0.75, 0.9, 0.95, 0.99, 0.999, 1.0};
    std::set<int_t> weights_seen;
    for (double q : quantiles) {
        int_t idx = q * graph.size() - 1;
        int_t weight = weights[idx];
        if (weights_seen.count(weight)) continue;
        weights_seen.insert(weight);
        idx = std::distance(weights.begin(), std::find(weights.begin(), weights.end(), weight));
        q = 1.0 * idx / graph.size();
        std::printf("graph_diagnostics  ::  Quantile %.3f node weight: %7d (%d nodes with >= this weight)\n", q, (int) weights[idx], (int) (graph.size() - idx));
    }
}

std::vector<int_t> get_neighborhood_size(const Graph& graph, int_t v, int_t max_depth) {
    std::vector<int_t> ne_size(max_depth + 1);
    std::vector<std::pair<int_t, int_t>> stack{{v, 0}};
    std::vector<bool> visited(graph.size());
    for (int_t d = 0, depth; d <= max_depth; ++d) {
        std::vector<std::pair<int_t, int_t>> next_depth;
        while (!stack.empty()) {
            std::tie(v, depth) = stack.back();
            stack.pop_back();
            if (visited[v]) continue;
            visited[v] = true;
            ++ne_size[d];
            if (depth == max_depth) continue;
            for (int_t w : graph[v].neighbors()) next_depth.emplace_back(w, depth + 1);
        }
        stack = std::move(next_depth);
    }
    for (int_t sz = 2; sz <= max_depth; ++sz) ne_size[sz] += ne_size[sz - 1];
    return ne_size;
}

void neighborhood_sizes(const Graph& graph, int_t max_depth) {
    using vi = std::vector<int_t>;
    std::vector<vi> ne_sizes(graph.size());
    for (int_t v = 0; v < graph.size(); ++v) ne_sizes[v] = get_neighborhood_size(graph, v, max_depth);
    for (auto sz = 2; sz <= max_depth; ++sz) {
        double avg_size = 1.0 * std::accumulate(ne_sizes.begin(), ne_sizes.end(), 0, [sz](int_t res, vi& ne_size){ return res + ne_size[sz]; }) / graph.size();
        int_t max_size = (*std::max_element(ne_sizes.begin(), ne_sizes.end(), [sz](vi& a, vi& b){ return a[sz] < b[sz]; }))[sz];
        std::printf("graph_diagnostics  ::  |ne(v)^%d|: avg %.1f, max %d\n", sz, avg_size, (int) max_size);
    }
}

void run_diagnostics(const Graph& graph, int_t max_depth) {
    vertex_degrees(graph);
    weight_distribution(graph);
    neighborhood_sizes(graph, max_depth);
}

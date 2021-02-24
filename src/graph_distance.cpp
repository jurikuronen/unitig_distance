/*
    This file contains various distance calculation routines.
*/

#include <iostream>
#include <map>
#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Couplings.hpp"
#include "Graph.hpp"
#include "graph_distance.hpp"
#include "Timer.hpp"
#include "types.hpp"

// Data structure for breadth-first searches.
struct bfs_task {
    int_t v;
    std::vector<int_t> ws;
    std::vector<int_t> coupling_indices;

    bfs_task(int_t v_) : v(v_) { }
    void add(int_t w, int_t idx) {
        ws.push_back(w);
        coupling_indices.push_back(idx);
    }
    std::size_t size() const { return ws.size(); }
};

// Compute number of queries for each vertex and aggregate into bfs tasks.
std::vector<bfs_task> compute_bfs_tasks(const Couplings& couplings) {
    std::vector<bfs_task> bfs_tasks;
    // Get queries for each vertex, storing also the coupling index.
    std::unordered_map<int_t, std::unordered_map<int_t, int_t>> queries;
    for (int_t i = 0; i < couplings.size(); ++i) {
        int_t v = couplings.v(i);
        int_t w = couplings.w(i);
        queries[v].emplace(w, i);
        queries[w].emplace(v, i);
    }
    // Compute number of queries and sort.
    std::vector<std::pair<int_t, int_t>> n_queries;
    for (const auto& q : queries) n_queries.emplace_back(q.second.size(), q.first);
    std::sort(n_queries.rbegin(), n_queries.rend());
    // Form bfs tasks.
    for (auto p : n_queries) {
        int_t v = p.second;
        if (queries[v].size() == 0) continue; // Overlapping queries were removed.
        bfs_task bt(v);
        for (auto q : queries[v]) {
            int_t w = q.first;
            int_t idx = q.second;
            bt.add(w, idx);
            queries[w].erase(v);
        }
        bfs_tasks.push_back(std::move(bt));
    }
    return bfs_tasks;
}

// Calculate distances for all couplings with a brute-force breadth-first search.
void calculate_distances_brute(const Graph& graph, const Couplings& couplings, const std::string& out_filename, Timer& timer, int_t n_threads, int_t block_size, int_t max_distance, bool verbose) {
    std::vector<int_t> res(couplings.size());
    auto lambda = [&graph, &couplings, &res, n_threads, max_distance](int_t thr, int_t start, int_t end) {
        for (auto i = thr + start; i < end; i += n_threads) {
            res[i] = distance_bfs(graph, couplings.v(i), couplings.w(i), max_distance);
        }
    };
    std::vector<std::thread> threads(n_threads);
    std::ofstream ofs(out_filename);

    for (int_t start = 0; start < couplings.size(); start += block_size) {
        Timer t;
        int_t end = std::min(start + block_size, (int_t) couplings.size());
        for (int_t thr = 0; thr < n_threads; ++thr) threads[thr] = std::thread(lambda, thr, start, end);
        for (auto& thr : threads) thr.join();
        for (int_t i = start; i < end; ++i) ofs << couplings.line(i) << ' ' << res[i] << '\n';
        std::cout << "calculate_distances_brute  ::  " << start + 1 << '-' << end << " / " << couplings.size() << " (" << n_threads << " threads)  ::  " << t.get_time_since_start() << "  ::  " << timer.get_time_since_start() << '\n';
    }
    std::cout << "calculate_distances_brute  ::  " << couplings.size() << " couplings ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << '\n';
}

// Calculate distances for all couplings with a smarter breadth-first search.
void calculate_distances_brute_smart(const Graph& graph, const Couplings& couplings, const std::string& out_filename, Timer& timer, int_t n_threads, int_t block_size, int_t max_distance, bool verbose) {
    auto bfs_tasks = compute_bfs_tasks(couplings);
    if (verbose) {
        std::cout << "calculate_distances_brute_smart  ::  Prepared " << bfs_tasks.size() << " bfs tasks.\n";
        std::cout << "calculate_distances_brute_smart  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << '\n';
    }
    std::vector<int_t> res(couplings.size());
    auto lambda = [&graph, &couplings, &res, &bfs_tasks, n_threads, max_distance](int_t thr, int_t start, int_t end) {
        for (auto i = thr + start; i < end; i += n_threads) {
            auto ws_dist = distance_bfs(graph, bfs_tasks[i].v, bfs_tasks[i].ws, max_distance); // In same order as ws in bfs_task.
            for (int_t w_idx = 0; w_idx < bfs_tasks[i].size(); ++w_idx) res[bfs_tasks[i].coupling_indices[w_idx]] = ws_dist[w_idx];
        }
    };
    std::vector<std::thread> threads(n_threads);
    std::ofstream ofs(out_filename);
    for (int_t start = 0; start < bfs_tasks.size(); start += block_size) {
        Timer t;
        int_t end = std::min(start + block_size, (int_t) bfs_tasks.size());
        for (int_t thr = 0; thr < n_threads; ++thr) threads[thr] = std::thread(lambda, thr, start, end);
        for (auto& thr : threads) thr.join();
        std::cout << "calculate_distances_brute_smart  ::  " << start + 1 << '-' << end << " / " << bfs_tasks.size() << " (" << n_threads << " threads)  ::  " << t.get_time_since_start() << "  ::  " << timer.get_time_since_start() << '\n';
    }
    for (int_t i = 0; i < couplings.size(); ++i) ofs << couplings.line(i) << ' ' << res[i] << '\n';
    std::cout << "calculate_distances_brute_smart  ::  Stored result for " << couplings.size() << " couplings ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << '\n';
}

// Compute distances between source and targets with a breadth-first search.
// Can constrain the search to not search further than a given distance by providing a max distance.
// If not provided, its default value is INT_T_MAX.
std::vector<int_t> distance_bfs(const Graph& graph, int_t source, const std::vector<int_t>& targets, int_t max_distance) {
    std::vector<int_t> dist(graph.size(), max_distance), is_target(graph.size());
    int_t targets_left = targets.size();
    for (auto w : targets) is_target[w] = true;
    dist[source] = graph[source].weight(); // Weights are computed by nodes rather than edges.
    std::set<std::pair<int_t, int_t>> queue{{dist[source], source}}; // (distance, node) pairs.
    while (!queue.empty()) {
        auto node = queue.begin()->second;
        if (is_target[node]) --targets_left;
        if (targets_left == 0) break; // Calculated distances for all targets.
        queue.erase(queue.begin());
        for (auto neighbor : graph[node].neighbors()) {
            auto weight = graph[neighbor].weight();
            if (dist[node] + weight < dist[neighbor]) {
                queue.erase({dist[neighbor], neighbor});
                dist[neighbor] = dist[node] + weight;
                queue.insert({dist[neighbor], neighbor});
            }
        }
    }
    std::vector<int_t> target_dist(targets.size());
    for (int_t i = 0; i < targets.size(); ++i) target_dist[i] = dist[targets[i]];
    return target_dist;
}

// Compute distance between source and destination with a breadth-first search.
// Can constrain the search to not search further than a given distance by providing a max distance.
// If not provided, its default value is INT_T_MAX.
int_t distance_bfs(const Graph& graph, int_t source, int_t destination, int_t max_distance) {
    if (graph[source].component_id() != graph[destination].component_id()) return -1;
    std::vector<int_t> dist(graph.size(), max_distance);
    dist[source] = graph[source].weight(); // Weights are computed by nodes rather than edges.
    std::set<std::pair<int_t, int_t>> queue{{dist[source], source}}; // (distance, node) pairs.
    while (!queue.empty()) {
        auto node = queue.begin()->second;
        if (node == destination) return dist[node];
        queue.erase(queue.begin());
        for (auto neighbor : graph[node].neighbors()) {
            auto weight = graph[neighbor].weight();
            if (dist[node] + weight < dist[neighbor]) {
                queue.erase({dist[neighbor], neighbor});
                dist[neighbor] = dist[node] + weight;
                queue.insert({dist[neighbor], neighbor});
            }
        }
    }
    return -1;
}


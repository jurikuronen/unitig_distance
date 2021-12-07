#include <iostream>
#include <map>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "Graph.hpp"
#include "graph_distance.hpp"
#include "search_job.hpp"
#include "SingleGenomeGraph.hpp"
#include "Timer.hpp"
#include "types.hpp"

using distance_tuple_t = std::tuple<real_t, real_t, real_t, int_t>;

static void update_source(std::vector<std::pair<int_t, real_t>>& sources, std::size_t mapped_idx, real_t distance) {
    auto it = sources.begin();
    while (it != sources.end() && it->first != (int_t) mapped_idx) ++it;
    if (it == sources.end()) sources.emplace_back(mapped_idx, distance);
    else it->second = std::min(it->second, distance);
}

static void add_sgg_source(const SingleGenomeGraph& sg_graph, std::vector<std::pair<int_t, real_t>>& sources, std::size_t v_original_idx) {
    auto v_path_idx = sg_graph.path_idx(v_original_idx);
    auto v_mapped_idx = sg_graph.mapped_idx(v_original_idx);
    if (v_path_idx == INT_T_MAX) {
        update_source(sources, v_mapped_idx, 0.0);
    } else {
        std::size_t path_endpoint;
        real_t distance;
        // Add path start node.
        std::tie(path_endpoint, distance) = sg_graph.distance_to_start(v_path_idx, v_mapped_idx);
        update_source(sources, path_endpoint, distance);
        // Add path end node.
        std::tie(path_endpoint, distance) = sg_graph.distance_to_end(v_path_idx, v_mapped_idx);
        update_source(sources, path_endpoint, distance);
    }
}

static std::vector<std::pair<int_t, real_t>> get_sgg_sources(const SingleGenomeGraph& sg_graph, int_t v) {
    std::vector<std::pair<int_t, real_t>> sources;
    add_sgg_source(sg_graph, sources, sg_graph.left_node(v));
    add_sgg_source(sg_graph, sources, sg_graph.right_node(v));
    return sources;
}

static void add_sgg_target(const SingleGenomeGraph& sg_graph, std::set<int_t>& target_set, std::size_t original_idx) {
    auto path_idx = sg_graph.path_idx(original_idx);
    auto mapped_idx = sg_graph.mapped_idx(original_idx);
    if (path_idx == INT_T_MAX) {
        target_set.insert(mapped_idx);
    } else {
        target_set.insert(sg_graph.start_node(path_idx));
        target_set.insert(sg_graph.end_node(path_idx));
    }
}

static std::vector<int_t> get_sgg_targets(const SingleGenomeGraph& sg_graph, const std::vector<int_t>& ws) {
    std::set<int_t> target_set;
    for (auto w : ws) {
        if (!sg_graph.contains(sg_graph.left_node(w))) continue;
        add_sgg_target(sg_graph, target_set, sg_graph.left_node(w));
        add_sgg_target(sg_graph, target_set, sg_graph.right_node(w));
    }
    std::vector<int_t> targets;
    for (auto t : target_set) targets.push_back(t);
    return targets;
}

static real_t get_correct_distance(
    const SingleGenomeGraph& sg_graph,
    std::size_t v_path_idx,
    std::size_t v_mapped_idx,
    std::size_t w_original_idx,
    std::map<std::size_t, real_t>& dist)
{
    auto w_path_idx = sg_graph.path_idx(w_original_idx);
    auto w_mapped_idx = sg_graph.mapped_idx(w_original_idx);
    if (w_path_idx == INT_T_MAX) return dist[w_mapped_idx]; // w not on path.
    if (v_path_idx == w_path_idx) return sg_graph.distance_in_path(v_path_idx, v_mapped_idx, w_mapped_idx); // v and w on same path.
    // w on path.
    std::size_t w_path_start, w_path_end;
    real_t w_path_start_distance, w_path_end_distance;
    std::tie(w_path_start, w_path_start_distance) = sg_graph.distance_to_start(w_path_idx, w_mapped_idx);
    std::tie(w_path_end, w_path_end_distance) = sg_graph.distance_to_end(w_path_idx, w_mapped_idx);
    return std::min(dist[w_path_start] + w_path_start_distance, dist[w_path_end] + w_path_end_distance);
}

static void process_job_distances(
    const SingleGenomeGraph& sg_graph,
    std::vector<real_t>& job_dist,
    std::size_t v_original_idx,
    const std::vector<int_t>& ws,
    std::map<std::size_t, real_t>& dist)
{
    auto v_path_idx = sg_graph.path_idx(v_original_idx);
    auto v_mapped_idx = sg_graph.mapped_idx(v_original_idx);
    for (std::size_t w_idx = 0; w_idx < ws.size(); ++w_idx) { 
        auto w = ws[w_idx];
        if (!sg_graph.contains(sg_graph.left_node(w))) continue;
        auto distance = get_correct_distance(sg_graph, v_path_idx, v_mapped_idx, sg_graph.left_node(w), dist);
        distance = std::min(distance, get_correct_distance(sg_graph, v_path_idx, v_mapped_idx, sg_graph.right_node(w), dist));
        job_dist[w_idx] = std::min(job_dist[w_idx], distance);
    }
}

static void add_job_distances_to_results(const search_job& job, const std::vector<real_t>& job_dist, std::vector<distance_tuple_t>& res) {
    for (std::size_t w_idx = 0; w_idx < job_dist.size(); ++w_idx) {
        auto distance = job_dist[w_idx];
        if (distance == REAL_T_MAX) continue;
        auto original_index = job.original_index(w_idx);
        real_t min, max, mean;
        int_t count;
        std::tie(min, max, mean, count) = res[original_index];
        min = std::min(min, distance);
        max = std::max(max, distance);
        mean = (mean * count + distance) / (count + 1);
        ++count;
        res[original_index] = std::make_tuple(min, max, mean, count);
    }
}

void calculate_sgg_distances(
    const SingleGenomeGraph& sg_graph,
    const std::vector<search_job>& search_jobs,
    std::vector<distance_tuple_t>& res,
    Timer& timer,
    int_t n_queries,
    int_t n_threads,
    int_t block_size,
    real_t max_distance)
{
    auto calculate_distance_block = [&sg_graph, &search_jobs, &res, n_threads, max_distance](std::size_t thr, std::size_t start, std::size_t end) {
        for (std::size_t i = thr + start; i < end; i += n_threads) {
            const auto& job = search_jobs[i];
            auto v = job.v();
            if (!sg_graph.contains(sg_graph.left_node(v))) continue;
            auto sources = get_sgg_sources(sg_graph, v);
            auto targets = get_sgg_targets(sg_graph, job.ws());
            auto target_dist = distance(sg_graph, sources, targets, max_distance);
            std::vector<real_t> job_dist(job.ws().size(), REAL_T_MAX);
            std::map<std::size_t, real_t> dist;
            for (std::size_t i = 0; i < targets.size(); ++i) dist[targets[i]] = target_dist[i];
            process_job_distances(sg_graph, job_dist, sg_graph.left_node(v), job.ws(), dist);
            process_job_distances(sg_graph, job_dist, sg_graph.right_node(v), job.ws(), dist);
            add_job_distances_to_results(job, job_dist, res);
        }
    };
    std::vector<std::thread> threads(n_threads);
    for (std::size_t start = 0; start < search_jobs.size(); start += block_size) {
        Timer t;
        std::size_t end = std::min(start + block_size, search_jobs.size());
        for (std::size_t thr = 0; thr < (std::size_t) n_threads; ++thr) threads[thr] = std::thread(calculate_distance_block, thr, start, end);
        for (auto& thr : threads) thr.join();
    }
}

std::vector<real_t> calculate_distances(
    const Graph& combined_graph,
    const std::vector<search_job>& search_jobs,
    Timer& timer,
    int_t n_queries,
    int_t n_threads,
    int_t block_size,
    real_t max_distance,
    bool verbose)
{
    std::vector<real_t> res(n_queries);
    auto calculate_distance_block = [&combined_graph, &search_jobs, &res, n_threads, max_distance](std::size_t thr, std::size_t start, std::size_t end) {
        for (std::size_t i = thr + start; i < end; i += n_threads) {
            const auto& job = search_jobs[i];
            std::vector<std::pair<int_t, real_t>> sources{{combined_graph.left_node(job.v()), 0.0}, {combined_graph.right_node(job.v()), 0.0}};
            std::vector<int_t> targets;
            for (auto w : job.ws()) {
                targets.push_back(combined_graph.left_node(w));
                targets.push_back(combined_graph.right_node(w));
            }
            auto target_dist = distance(combined_graph, sources, targets, max_distance);
            for (std::size_t w_idx = 0; w_idx < job.size(); ++w_idx) res[job.original_index(w_idx)] = std::min(target_dist[2 * w_idx], target_dist[2 * w_idx + 1]);
        }
    };
    std::vector<std::thread> threads(n_threads);
    for (std::size_t start = 0; start < search_jobs.size(); start += block_size) {
        Timer t;
        std::size_t end = std::min(start + block_size, search_jobs.size());
        for (std::size_t thr = 0; thr < (std::size_t) n_threads; ++thr) threads[thr] = std::thread(calculate_distance_block, thr, start, end);
        for (auto& thr : threads) thr.join();
        if (verbose) std::cout << timer.get_time_block_since_start() << " Calculated distances for block " << start + 1 << '-' << end << " / " << search_jobs.size() << " in " << t.get_time_since_mark() << "." << std::endl;
    }
    return res;
}

// Compute shortest distance between source(s) and targets. Can constrain the search to stop at max distance (set to REAL_T_MAX by default).
std::vector<real_t> distance(const Graph& graph, const std::vector<std::pair<int_t, real_t>>& sources, const std::vector<int_t>& targets, real_t max_distance) {
    std::vector<real_t> dist(graph.size(), max_distance);
    std::vector<bool> is_target(graph.size());
    for (auto w : targets) is_target[w] = true;
    std::set<std::pair<real_t, int_t>> queue; // (distance, node) pairs.
    for (auto s : sources) {
        dist[s.first] = s.second;
        queue.emplace(s.second, s.first);
    }
    int_t targets_left = targets.size(), neighbor_idx;
    real_t weight;
    while (!queue.empty()) {
        auto node = queue.begin()->second;
        queue.erase(queue.begin());
        if (is_target[node]) {
            --targets_left;
            is_target[node] = false;
        }
        if (targets_left == 0) break; // Calculated distances for all targets.
        for (auto neighbor : graph[node].neighbors()) {
            std::tie(neighbor_idx, weight) = neighbor;
            if (dist[node] + weight < dist[neighbor_idx]) {
                queue.erase({dist[neighbor_idx], neighbor_idx});
                dist[neighbor_idx] = dist[node] + weight;
                queue.insert({dist[neighbor_idx], neighbor_idx});
            }
        }
    }
    std::vector<real_t> target_dist;
    for (auto target : targets) target_dist.push_back(dist[target]);
    return target_dist;
}


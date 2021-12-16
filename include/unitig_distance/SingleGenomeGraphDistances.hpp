#pragma once

#include <iostream>
#include <map>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "Graph.hpp"
#include "SearchJobs.hpp"
#include "Timer.hpp"
#include "types.hpp"

using distance_tuple_t = std::tuple<real_t, real_t, real_t, int_t>;

class SingleGenomeGraphDistances {
public:
    SingleGenomeGraphDistances(
        const SingleGenomeGraph& graph,
        std::vector<distance_tuple_t>& sgg_distances,
        int_t n_threads,
        int_t block_size,
        real_t max_distance = REAL_T_MAX,
        bool verbose = false)
    : m_graph(graph),
      m_sgg_distances(sgg_distances),
      m_n_threads(n_threads),
      m_block_size(block_size),
      m_max_distance(max_distance)
    { }

    // Calculate distances for single genome graphs.
    void solve(const SearchJobs& search_jobs) {
        auto calculate_distance_block = [this, &search_jobs](std::size_t thr, std::size_t block_start, std::size_t block_end) {
            for (std::size_t i = thr + block_start; i < block_end; i += m_n_threads) {
                const auto& job = search_jobs[i];

                auto v = job.v();
                if (!m_graph.contains(m_graph.left_node(v))) continue;

                auto sources = get_sgg_sources(v);
                auto targets = get_sgg_targets(job.ws());
                auto target_dist = m_graph.distance(sources, targets, m_max_distance);

                std::vector<real_t> job_dist(job.ws().size(), m_max_distance);
                std::map<int_t, real_t> dist;
                for (std::size_t i = 0; i < targets.size(); ++i) dist[targets[i]] = target_dist[i];
                process_job_distances(job_dist, m_graph.left_node(v), job.ws(), dist);
                process_job_distances(job_dist, m_graph.right_node(v), job.ws(), dist);
                add_job_distances_to_sgg_distances(job, job_dist);
            }
        };
        std::vector<std::thread> threads(m_n_threads);
        for (std::size_t start = 0; start < search_jobs.size(); start += m_block_size) {
            std::size_t end = std::min(start + m_block_size, search_jobs.size());
            for (std::size_t thr = 0; thr < (std::size_t) m_n_threads; ++thr) threads[thr] = std::thread(calculate_distance_block, thr, start, end);
            for (auto& thr : threads) thr.join();
        }
    }

private:
    const SingleGenomeGraph& m_graph;

    std::vector<distance_tuple_t>& m_sgg_distances;

    int_t m_n_threads;
    int_t m_block_size;
    real_t m_max_distance;

    void update_source(std::vector<std::pair<int_t, real_t>>& sources, int_t mapped_idx, real_t distance) {
        auto it = sources.begin();
        while (it != sources.end() && it->first != (int_t) mapped_idx) ++it;
        if (it == sources.end()) sources.emplace_back(mapped_idx, distance);
        else it->second = std::min(it->second, distance);
    }

    void add_sgg_source(std::vector<std::pair<int_t, real_t>>& sources, int_t v_original_idx) {
        auto v_path_idx = m_graph.path_idx(v_original_idx);
        auto v_mapped_idx = m_graph.mapped_idx(v_original_idx);
        if (v_path_idx == INT_T_MAX) {
            update_source(sources, v_mapped_idx, 0.0);
        } else {
            int_t path_endpoint;
            real_t distance;
            // Add path start node.
            std::tie(path_endpoint, distance) = m_graph.distance_to_start(v_path_idx, v_mapped_idx);
            update_source(sources, path_endpoint, distance);
            // Add path end node.
            std::tie(path_endpoint, distance) = m_graph.distance_to_end(v_path_idx, v_mapped_idx);
            update_source(sources, path_endpoint, distance);
        }
    }

    std::vector<std::pair<int_t, real_t>> get_sgg_sources(int_t v) {
        std::vector<std::pair<int_t, real_t>> sources;
        add_sgg_source(sources, m_graph.left_node(v));
        add_sgg_source(sources, m_graph.right_node(v));
        return sources;
    }

    void add_sgg_target(std::set<int_t>& target_set, int_t original_idx) {
        auto path_idx = m_graph.path_idx(original_idx);
        auto mapped_idx = m_graph.mapped_idx(original_idx);
        if (path_idx == INT_T_MAX) {
            target_set.insert(mapped_idx);
        } else {
            target_set.insert(m_graph.start_node(path_idx));
            target_set.insert(m_graph.end_node(path_idx));
        }
    }

    std::vector<int_t> get_sgg_targets(const std::vector<int_t>& ws) {
        std::set<int_t> target_set;
        for (auto w : ws) {
            if (!m_graph.contains(m_graph.left_node(w))) continue;
            add_sgg_target(target_set, m_graph.left_node(w));
            add_sgg_target(target_set, m_graph.right_node(w));
        }
        std::vector<int_t> targets;
        for (auto t : target_set) targets.push_back(t);
        return targets;
    }

    real_t get_correct_distance(
        int_t v_path_idx,
        int_t v_mapped_idx,
        int_t w_original_idx,
        std::map<int_t, real_t>& dist)
    {
        auto w_path_idx = m_graph.path_idx(w_original_idx);
        auto w_mapped_idx = m_graph.mapped_idx(w_original_idx);
        if (w_path_idx == INT_T_MAX) return dist[w_mapped_idx]; // w not on path.
        if (v_path_idx == w_path_idx) return m_graph.distance_in_path(v_path_idx, v_mapped_idx, w_mapped_idx); // v and w on same path.
        // w on path.
        int_t w_path_start, w_path_end;
        real_t w_path_start_distance, w_path_end_distance;
        std::tie(w_path_start, w_path_start_distance) = m_graph.distance_to_start(w_path_idx, w_mapped_idx);
        std::tie(w_path_end, w_path_end_distance) = m_graph.distance_to_end(w_path_idx, w_mapped_idx);
        return std::min(dist[w_path_start] + w_path_start_distance, dist[w_path_end] + w_path_end_distance);
    }

    void process_job_distances(
        std::vector<real_t>& job_dist,
        int_t v_original_idx,
        const std::vector<int_t>& ws,
        std::map<int_t, real_t>& dist)
    {
        auto v_path_idx = m_graph.path_idx(v_original_idx);
        auto v_mapped_idx = m_graph.mapped_idx(v_original_idx);
        for (std::size_t w_idx = 0; w_idx < ws.size(); ++w_idx) { 
            auto w = ws[w_idx];
            if (!m_graph.contains(m_graph.left_node(w))) continue;
            auto distance = get_correct_distance(v_path_idx, v_mapped_idx, m_graph.left_node(w), dist);
            distance = std::min(distance, get_correct_distance(v_path_idx, v_mapped_idx, m_graph.right_node(w), dist));
            job_dist[w_idx] = std::min(job_dist[w_idx], distance);
        }
    }

    void add_job_distances_to_sgg_distances(const SearchJob& job, const std::vector<real_t>& job_dist) {
        for (std::size_t w_idx = 0; w_idx < job_dist.size(); ++w_idx) {
            auto distance = job_dist[w_idx];
            if (distance >= m_max_distance) continue;
            auto original_index = job.original_index(w_idx);
            real_t min, max, mean;
            int_t count;
            std::tie(min, max, mean, count) = m_sgg_distances[original_index];
            min = std::min(min, distance);
            max = std::max(max, distance);
            mean = (mean * count + distance) / (count + 1);
            ++count;
            m_sgg_distances[original_index] = std::make_tuple(min, max, mean, count);
        }
    }


};
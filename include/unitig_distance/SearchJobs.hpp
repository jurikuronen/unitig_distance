#pragma once

#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Queries.hpp"
#include "types.hpp"

// Distance queries for node v.
class SearchJob {
public:
    SearchJob(int_t v) : m_v(v)  { }
    const int_t v() const { return m_v; }
    const std::vector<int_t>& ws() const { return m_ws; }
    const int_t original_index(std::size_t idx) const { return m_original_indices[idx]; }
    void add(int_t w, int_t idx) {
        m_ws.push_back(w);
        m_original_indices.push_back(idx);
    }
    std::size_t size() const { return m_ws.size(); }

private:
    int_t m_v;
    std::vector<int_t> m_ws;
    std::vector<int_t> m_original_indices;

};

// Clean up later.
class SearchJobs {
public:
    SearchJobs(const Queries& queries) : m_n_queries(queries.size()) {
        // Store queries by vertex, storing also the original indices.
        std::unordered_map<int_t, std::unordered_map<int_t, int_t>> queries_map;
        for (std::size_t idx = 0; idx < queries.size(); ++idx) {
            auto v = queries.v(idx);
            auto w = queries.w(idx);
            queries_map[v].emplace(w, idx);
            queries_map[w].emplace(v, idx);
        }
        // Get query counts for the vertices.
        std::set<std::pair<int_t, int_t>> n_queries_set; // (v_n_queries, v)
        std::unordered_map<int_t, int_t> n_queries; // (v, v_n_queries).
        for (const auto& q : queries_map) {
            int_t v = q.first;
            int_t v_n_queries = (int_t) q.second.size();
            n_queries_set.emplace(v_n_queries, v);
            n_queries.emplace(v, v_n_queries);
        }
        // Calculate optimal search jobs.
        while (n_queries_set.rbegin()->first != 0) { // Always points to largest value (v with most queries).
            auto it = std::prev(n_queries_set.end());
            int_t v = it->second;
            // Can already remove query count trackers for v here.
            n_queries_set.erase(it);
            n_queries[v] = 0;
            SearchJob job(v);
            // Add remaining (v, w) queries for v.
            for (auto q : queries_map[v]) {
                int_t w, idx;
                std::tie(w, idx) = q;
                job.add(w, idx);
                queries_map[w].erase(v); // Remove duplicate query (w, v).
                int_t old_n_queries = n_queries[w]; // Get w's old query counts.
                // Update query count trackers for w.
                n_queries_set.erase(std::make_pair(old_n_queries, w));
                n_queries_set.emplace(old_n_queries - 1, w);
                --n_queries[w];
            }
            m_search_jobs.push_back(std::move(job));
        }
    }

    std::size_t size() const { return m_search_jobs.size(); }

    int_t n_queries() const { return m_n_queries; }

    SearchJob& operator[](std::size_t idx) { return m_search_jobs[idx]; }
    const SearchJob& operator[](std::size_t idx) const { return m_search_jobs[idx]; }

private:
    std::vector<SearchJob> m_search_jobs;

    int_t m_n_queries;

};

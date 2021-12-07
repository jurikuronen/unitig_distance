#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "search_job.hpp"
#include "SpydrPickOutput.hpp"
#include "types.hpp"

std::vector<search_job> compute_search_jobs(const SpydrPickOutput& spydrpick_output) {
    std::vector<search_job> search_jobs;
    // Get queries for each vertex, storing also the original indices.
    std::unordered_map<int_t, std::unordered_map<int_t, int_t>> queries;
    for (std::size_t idx = 0; idx < spydrpick_output.size(); ++idx) {
        int_t v = spydrpick_output.v(idx);
        int_t w = spydrpick_output.w(idx);
        queries[v].emplace(w, idx);
        queries[w].emplace(v, idx);
    }
    // Track number of queries.
    std::set<std::pair<int_t, int_t>> query_size_to_node;
    std::unordered_map<int_t, int_t> n_queries;
    for (const auto& q : queries) query_size_to_node.emplace(q.second.size(), q.first);
    for (auto q : query_size_to_node) n_queries[q.second] = q.first;
    // Form search jobs.
    while (query_size_to_node.rbegin()->first != 0) { // Always points to largest value.
        auto it = std::prev(query_size_to_node.end());
        int_t v = it->second;
        // Can already remove query trackers for v here.
        query_size_to_node.erase(it);
        n_queries[v] = 0;
        search_job job(v);
        for (auto q : queries[v]) {
            int_t w = q.first;
            int_t idx = q.second;
            job.add(w, idx);
            queries[w].erase(v); // Remove duplicate query.
            int_t old_sz = n_queries[w]; // Get w's old query size.
            // Update query size trackers for w.
            query_size_to_node.erase(std::make_pair(old_sz, w));
            query_size_to_node.emplace(old_sz - 1, w);
            --n_queries[w];
        }
        search_jobs.push_back(std::move(job));
    }
    return search_jobs;
}

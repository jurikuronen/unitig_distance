#pragma once

#include <string>
#include <tuple>
#include <vector>

#include "Couplings.hpp"
#include "Graph.hpp"
#include "search_job.hpp"
#include "SingleGenomeGraph.hpp"
#include "Timer.hpp"
#include "types.hpp"

void calculate_sgg_distances(
    const SingleGenomeGraph& sg_graph,
    const std::vector<search_job>& search_jobs,
    std::vector<std::tuple<real_t, real_t, real_t, int_t>>& res,
    Timer& timer,
    int_t n_couplings,
    int_t n_threads,
    int_t block_size,
    real_t max_distance = REAL_T_MAX);

std::vector<real_t> calculate_distances(
    const Graph& graph,
    const std::vector<search_job>& search_jobs,
    Timer& timer,
    int_t n_couplings,
    int_t n_threads,
    int_t block_size,
    real_t max_distance = REAL_T_MAX,
    bool verbose = false);

// Compute shortest distance between source(s) and targets. Can constrain the search to stop at max distance (set to REAL_T_MAX by default).
std::vector<real_t> distance(const Graph& graph, const std::vector<std::pair<int_t, real_t>>& sources, const std::vector<int_t>& targets, real_t max_distance = REAL_T_MAX);


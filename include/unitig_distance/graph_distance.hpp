#pragma once

#include <string>
#include <vector>

#include "Couplings.hpp"
#include "Graph.hpp"
#include "Timer.hpp"
#include "types.hpp"

// Data structure for searches.
struct search_task;

// Compute number of queries for each vertex and aggregate into search tasks.
std::vector<search_task> compute_search_tasks(const Couplings& couplings);

// Calculate distances for all couplings with a brute-force search.
void calculate_distances_brute(const Graph& graph, const Couplings& couplings, const std::string& out_filename, Timer& timer, int_t n_threads, int_t block_size, int_t max_distance = INT_T_MAX, bool verbose = false);

// Calculate distances for all couplings with a smarter search.
void calculate_distances_brute_smart(const Graph& graph, const Couplings& couplings, const std::string& out_filename, Timer& timer, int_t n_threads, int_t block_size, int_t max_distance = INT_T_MAX, bool verbose = false);

// Compute distances between source and targets.
// Can constrain the search to not search further than a given distance by providing a max distance.
// If not provided, its default value is INT_T_MAX.
std::vector<int_t> distance(const Graph& graph, int_t source, const std::vector<int_t>& targets, int_t max_distance);

// Compute distance between source and destination.
// Can constrain the search to not search further than a given distance by providing a max distance.
// If not provided, its default value is INT_T_MAX.
int_t distance(const Graph& graph, int_t source, int_t destination, int_t max_distance = INT_T_MAX);

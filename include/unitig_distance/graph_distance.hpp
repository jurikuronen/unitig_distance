#pragma once

#include <string>
#include <vector>

#include "Couplings.hpp"
#include "Graph.hpp"
#include "Timer.hpp"
#include "types.hpp"

// Data structure for breadth-first searches.
struct bfs_task;
// Compute number of queries for each vertex and aggregate into bfs tasks.
std::vector<bfs_task> compute_bfs_tasks(const Couplings& couplings);

// Compute distances for all couplings.
void calculate_distances_brute(const Graph& graph, const Couplings& couplings, const std::string& out_filename, Timer& timer, int_t n_threads, int_t block_size, int_t max_distance = INT_T_MAX, bool verbose = false);

// Calculate distances for all couplings with a smarter breadth first search.
void calculate_distances_brute_smart(const Graph& graph, const Couplings& couplings, const std::string& out_filename, Timer& timer, int_t n_threads, int_t block_size, int_t max_distance = INT_T_MAX, bool verbose = false);

// Compute distances between source and targets with a breadth-first search.
// Can constrain the search to not search further than a given distance by providing a max distance.
// If not provided, its default value is INT_T_MAX.
std::vector<int_t> distance_bfs(const Graph& graph, int_t source, const std::vector<int_t>& targets, int_t max_distance);

// Compute distance between source and destination with a breadth first search.
// Can constrain the search to not search further than a given distance by providing a max distance.
// If not provided, its default value is INT_T_MAX.
int_t distance_bfs(const Graph& graph, int_t source, int_t destination, int_t max_distance = INT_T_MAX);

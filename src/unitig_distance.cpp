/*
    Main function.
*/

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "Couplings.hpp"
#include "Graph.hpp"
#include "graph_algorithms.hpp"
#include "Program_options.hpp"
#include "Timer.hpp"
#include "types.hpp"

int main(int argc, char** argv) {
    std::cout << "unitig_distance | MIT License | Copyright (c) 2021 Juri Kuronen\n\n";

    // Read command line arguments.
    Program_options po(argc, argv);
    if (!po.valid_state()) return 1;

    Timer timer;

    // Read graph from provided files.
    Graph graph(po.nodes_filename(), po.edges_filename());
    if (po.verbose()) {
        int_t n_edges = 0, max_degree = 0;
        for (const auto& node : graph) {
            int_t sz = node.neighbors().size();
            n_edges += sz;
            max_degree = std::max(max_degree, sz);
        }
        if (graph.one_based()) std::cout << "Graph  ::  Detected that graph files were one-based.\n";
        std::cout << "Graph  ::  Graph has " << graph.size() << " nodes and " << n_edges / 2 << " edges. Max degree is " << max_degree << ".\n";
        std::cout << "Graph  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << '\n';
    }

    // Connected components.
    graph.solve_connected_components();
    if (po.verbose())  {
        std::vector<int_t> component_sizes;
        std::map<int_t, int_t> component_sizes_map;
        for (const auto& node : graph) ++component_sizes_map[node.component_id()];
        for (auto p : component_sizes_map) component_sizes.push_back(p.second);
        std::sort(component_sizes.rbegin(), component_sizes.rend());
        std::cout << "solve_connected_components  ::  Found " << component_sizes.size() << " connected components. Top 5 largest:";
        for (auto i = 0; i < std::min<std::size_t>(5, component_sizes.size()); ++i) std::cout << ' ' << component_sizes[i];
        std::cout << ".\n";
        std::cout << "solve_connected_components  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << '\n';
    }

    // Biconnected components.
    auto blocks = graph.solve_biconnected_components();
    if (po.verbose()) {
        int_t n_articulation_points = 0;
        for (const auto& node : graph) n_articulation_points += node.is_articulation_point();
        std::cout << "solve_biconnected_components  ::  Found " << n_articulation_points << " articulation points.\n";
        std::vector<int_t> block_sizes;
        for (const auto& block : blocks) block_sizes.push_back(block.size());
        std::sort(block_sizes.rbegin(), block_sizes.rend());
        std::cout << "solve_biconnected_components  ::  Found " << blocks.size() << " biconnected components. Top 5 largest:";
        for (auto i = 0; i < std::min<std::size_t>(5, block_sizes.size()); ++i) std::cout << ' ' << block_sizes[i];
        std::cout << ".\n";
        std::cout << "solve_biconnected_components  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << '\n';
    }

    // Triconnected components and cut vertices.
    for (const auto& block : blocks) {
        if (block.size() <= 50) continue; // No need to process such small blocks.
        Graph subgraph(graph, block);
        auto cv = solve_cut_vertices(subgraph);
        for (auto v : cv) graph[v].set_cut_node();
    }
    if (po.verbose()) {
        int_t n_cut_vertices = 0;
        for (const auto& node : graph) n_cut_vertices += node.is_cut_node();
        std::cout << "solve_cut_vertices  ::  Found " << n_cut_vertices << " cut vertices (before optimization).\n";
        std::cout << "solve_cut_vertices  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << '\n';
    }

    // Cut vertex optimization routines.
    optimize_cut_vertices_along_paths(graph);
    if (po.verbose()) {
        int_t n_cut_vertices = 0;
        for (const auto& node : graph) n_cut_vertices += node.is_cut_node();
        std::cout << "optimize_cut_vertices  ::  Cleaned paths, " << n_cut_vertices << " cut vertices left after optimization.\n";
        std::cout << "optimize_cut_vertices  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << '\n';
    }

    // Read couplings and calculate distances.
    Couplings couplings(po.couplings_filename(), po.n_couplings(), po.one_based());
    po.set_n_couplings(couplings.size());
    if (po.verbose()) {
        std::cout << "Couplings  ::  Read " << couplings.size() << " couplings.\n";
        std::cout << "Couplings  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << '\n';
    }

    // Calculate distances.
    if (po.use_smart_search()) {
        calculate_distances_brute_smart(graph, couplings, po.out_filename(), timer, po.n_threads(), po.block_size(), po.max_distance(), po.verbose());
    } else {
        calculate_distances_brute(graph, couplings, po.out_filename(), timer, po.n_threads(), po.block_size(), po.max_distance(), po.verbose());
    }
    std::cout << "unitig_distance  ::  " << timer.get_time_since_start() << '\n';
}

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
        if (graph.one_based()) std::cout << "Graph  ::  Detected that graph files were one-based." << std::endl;
        std::cout << "Graph  ::  Graph has " << neat_number_str(graph.size()) << " nodes and " << neat_number_str(n_edges / 2) << " edges. Max degree is " << neat_number_str(max_degree) << '.' << std::endl;
        std::cout << "Graph  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << std::endl;
    }

    if (po.repeating_filename() != "") {
        auto repeating_filter_result = graph.apply_repeating_unitigs_filter(po.repeating_filename(), po.repeating_criterion());
        if (po.verbose()) {
            int_t removed_nodes = repeating_filter_result.first, removed_edges = repeating_filter_result.second;
            std::cout << "Graph  ::  Applied repeating unitigs filter (criterion: >=" << po.repeating_criterion() << ") and disconnected " << neat_number_str(removed_nodes) << " nodes by removing " << neat_number_str(removed_edges) << " edges." << std::endl;
            int_t n_edges = 0, max_degree = 0;
            for (const auto& node : graph) {
                int_t sz = node.neighbors().size();
                n_edges += sz;
                max_degree = std::max(max_degree, sz);
            }
            std::cout << "Graph  ::  Graph has " << neat_number_str(graph.size() - removed_nodes) << " unaffected nodes and " << neat_number_str(n_edges / 2) << " edges left. Max degree is " << neat_number_str(max_degree) << '.' << std::endl;
            std::cout << "Graph  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << std::endl;

        }
    }

    if (po.run_diagnostics()) {
        run_diagnostics(graph, po.graph_diagnostics_depth());
        std::cout << "graph_diagnostics  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << std::endl;
    }

    std::cout << "********************************************************************************" << std::endl;
    std::cout << "*** Note: The decomposition calculated below is currently not used.          ***" << std::endl;
    std::cout << "***       Printing statistics for informational purposes only.               ***" << std::endl;
    std::cout << "********************************************************************************" << std::endl;

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
        std::cout << '.' << std::endl;
        std::cout << "solve_connected_components  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << std::endl;
    }

    // Biconnected components.
    auto blocks = graph.solve_biconnected_components();
    if (po.verbose()) {
        int_t n_articulation_points = 0;
        for (const auto& node : graph) n_articulation_points += node.is_articulation_point();
        std::cout << "solve_biconnected_components  ::  Found " << n_articulation_points << " articulation points." << std::endl;
        std::vector<int_t> block_sizes;
        for (const auto& block : blocks) block_sizes.push_back(block.size());
        std::sort(block_sizes.rbegin(), block_sizes.rend());
        std::cout << "solve_biconnected_components  ::  Found " << blocks.size() << " biconnected components. Top 5 largest:";
        for (auto i = 0; i < std::min<std::size_t>(5, block_sizes.size()); ++i) std::cout << ' ' << block_sizes[i];
        std::cout << '.' << std::endl;
        std::cout << "solve_biconnected_components  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << std::endl;
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
        std::cout << "solve_cut_vertices  ::  Found " << n_cut_vertices << " cut vertices (before optimization)." << std::endl;
        std::cout << "solve_cut_vertices  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << std::endl;
    }

    // Cut vertex optimization routines.
    optimize_cut_vertices_along_paths(graph);
    if (po.verbose()) {
        int_t n_cut_vertices = 0;
        for (const auto& node : graph) n_cut_vertices += node.is_cut_node();
        std::cout << "optimize_cut_vertices  ::  Cleaned paths, " << n_cut_vertices << " cut vertices left after optimization." << std::endl;
        std::cout << "optimize_cut_vertices  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << std::endl;
    }

    std::cout << "********************************************************************************" << std::endl;

    if (po.n_couplings() > 0) {
        // Read couplings.
        Couplings couplings(po.couplings_filename(), po.n_couplings(), po.one_based());
        po.set_n_couplings(couplings.size());
        if (po.verbose()) {
            std::cout << "Couplings  ::  Read " << neat_number_str(couplings.size()) << " couplings." << std::endl;
            std::cout << "Couplings  ::  " << timer.get_time_since_mark() << "  ::  " << timer.get_time_since_start_and_set_mark() << std::endl;
        }

        // Calculate distances.
        if (po.use_smart_search()) {
            calculate_distances_brute_smart(graph, couplings, po.out_filename(), timer, po.n_threads(), po.block_size(), po.max_distance(), po.verbose());
        } else {
            calculate_distances_brute(graph, couplings, po.out_filename(), timer, po.n_threads(), po.block_size(), po.max_distance(), po.verbose());
        }
    }
    std::cout << "unitig_distance  ::  " << timer.get_time_since_start() << std::endl;
}

/*
    Main function.
*/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Graph.hpp"
#include "graph_algorithms.hpp"
#include "Timer.hpp"
#include "types.hpp"

int main(int argc, char** argv) {
    std::string nodes_filename(argv[1]);
    std::string edges_filename(argv[2]);
    std::string couplings_filename(argv[3]);
    Timer timer;
    Graph graph(nodes_filename, edges_filename);
    std::cout << "construct_graph  ::  " << timer.get_time_since_mark_and_set_mark() << '\n';
    graph.solve_connected_components();
    std::cout << "solve_connected_components  ::  " << timer.get_time_since_mark_and_set_mark() << '\n';
    auto blocks = graph.solve_biconnected_components();
    std::cout << "solve_biconnected_components  ::  " << timer.get_time_since_mark_and_set_mark() << '\n';
    for (const auto& block : blocks) {
        if (block.size() <= 50) continue; // No need to process such small blocks.
        Graph subgraph(graph, block);
        auto cv = solve_cut_vertices(subgraph);
        for (auto v : cv) graph[v].set_cut_node();
    }
    #ifdef DEBUG
        int_t n_cut_vertices = 0;
        for (const auto& node : graph) n_cut_vertices += node.is_cut_node();
        std::cout << "Found " << n_cut_vertices << " cut vertices (before optimization).\n";
    #endif
    std::cout << "solve_cut_vertices ::  " << timer.get_time_since_mark_and_set_mark() << '\n';
    optimize_cut_vertices_along_paths(graph);
    #ifdef DEBUG
        n_cut_vertices = 0;
        for (const auto& node : graph) n_cut_vertices += node.is_cut_node();
        std::cout << "Cleaned paths, " << n_cut_vertices << " cut vertices left after optimization.\n";
    #endif
    std::cout << "optimize_cut_vertices_along_paths ::  " << timer.get_time_since_mark_and_set_mark() << '\n';
    return 0;
    std::ifstream ifs(couplings_filename);
    std::string line;
    int_t v, w;
    while (std::getline(ifs, line)) {
        std::stringstream ss(line);
        ss >> v >> w;
        std::cout << line << ' ' << distance_naive(graph, v, w) << '\n';
    }
}

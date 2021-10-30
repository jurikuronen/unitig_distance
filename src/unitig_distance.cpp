#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Couplings.hpp"
#include "Graph.hpp"
#include "graph_distance.hpp"
#include "Program_options.hpp"
#include "search_job.hpp"
#include "SingleGenomeGraph.hpp"
#include "Timer.hpp"
#include "types.hpp"

using distance_tuple_t = std::tuple<real_t, real_t, real_t, int_t>;

bool check_file(const std::string& filename) {
    auto ok = std::ifstream(filename).good();
    if (!ok) std::cout << "Can't open file " << filename << "\n";
    return ok;
}

bool sanity_check_input_files(const Program_options& po) {
    bool ok = check_file(po.nodes_filename()) && check_file(po.edges_filename());
    if (po.n_couplings() > 0) ok &= check_file(po.couplings_filename());    
    if (ok) {
        if (po.paths_filename() != "") ok &= check_file(po.paths_filename());
        std::ifstream ifs(po.paths_filename());
        for (std::string path_edges; ok && std::getline(ifs, path_edges); ok &= check_file(path_edges));
    }
    return ok;
}

std::string neat_number_str(int_t number) {
    std::vector<int_t> parts;
    do parts.push_back(number % 1000);
    while (number /= 1000);
    std::string number_str = std::to_string(parts.back());
    for (int_t i = parts.size() - 2; i >= 0; --i) {
        number_str += ' ' + std::string(3 - std::to_string(parts[i]).size(), '0') + std::to_string(parts[i]);
    }
    return number_str;
}

int main(int argc, char** argv) {
    Timer timer;

    std::cout << "unitig_distance | MIT License | Copyright (c) 2021 Juri Kuronen\n\n";

    // Read command line arguments.
    Program_options po(argc, argv);
    if (!po.valid_state()) return 1;
    if (!sanity_check_input_files(po)) return 1;

    // Construct the combined graph from provided files.
    Graph combined_graph(po.nodes_filename(), po.edges_filename(), po.k());
    if (po.verbose()) {
        int_t n_nodes, n_edges, max_degree;
        std::tie(n_nodes, n_edges, max_degree) = combined_graph.get_details();
        std::cout << timer.get_time_block_since_start() << " Created combined graph in " << timer.get_time_since_mark_and_set_mark() 
                  << ". The graph has " << neat_number_str(n_nodes) << " connected (half) nodes and " << neat_number_str(n_edges) 
                  << " edges. Max degree is " << neat_number_str(max_degree) << "." << std::endl;
    }

    // Calculate coupling distances in the graph(s).
    if (po.n_couplings() > 0) {
        // Read couplings.
        Couplings couplings(po.couplings_filename(), po.n_couplings(), po.one_based());
        if (po.print_unitigs()) couplings.read_unitigs(po.nodes_filename());
        po.set_n_couplings(couplings.size());
        if (po.verbose()) {
            std::cout << timer.get_time_block_since_start() << " Read " << neat_number_str(couplings.size()) << " couplings in "
                      << timer.get_time_since_mark_and_set_mark() << "." << std::endl;
        }

        // Compute search jobs.
        auto search_jobs = compute_search_jobs(couplings);
        if (po.verbose()) {
            std::cout << timer.get_time_block_since_start() << " Prepared " << neat_number_str(search_jobs.size()) << " search jobs in "
                      << timer.get_time_since_mark_and_set_mark() << "." << std::endl;
        }

        if (!po.run_sggs_only()) {
            // Calculate distances in the combined graph.
            if (po.verbose()) std::cout << timer.get_time_block_since_start_and_set_mark() << " Calculating distances in the combined graph." << std::endl;
            std::vector<real_t> graph_distances = calculate_distances(combined_graph,
                                                                      search_jobs,
                                                                      timer,
                                                                      couplings.size(),
                                                                      po.n_threads(),
                                                                      po.block_size(),
                                                                      po.max_distance(),
                                                                      po.verbose());

            // Output distances for the combined graph.
            couplings.output_distances(po.out_cg_filename(), graph_distances);
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output combined graph distances to file " << po.out_cg_filename()
                                        << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;

            if (po.filter_filename() != "") {
                // Create filtered graph by applying filter to the combined graph.
                auto filtered_graph = Graph(combined_graph, po.filter_filename(), po.filter_criterion());
                if (po.verbose()) {
                    int_t n_nodes, n_edges, max_degree;
                    std::tie(n_nodes, n_edges, max_degree) = combined_graph.get_details();
                    std::cout << timer.get_time_block_since_start_and_set_mark() << " Created filtered graph (criterion: >=" << po.filter_criterion()
                              << ") from the combined graph. The graph has " << neat_number_str(n_nodes) << " connected (half) nodes and "
                              << neat_number_str(n_edges) << " edges (max degree " << neat_number_str(max_degree) << ")." << std::endl;
                }
                // Calculate distances in the filtered graph.
                if (po.verbose()) std::cout << timer.get_time_block_since_start_and_set_mark() << " Calculating distances in the filtered graph." << std::endl;
                graph_distances = calculate_distances(filtered_graph,
                                                      search_jobs,
                                                      timer,
                                                      couplings.size(),
                                                      po.n_threads(),
                                                      po.block_size(),
                                                      po.max_distance(),
                                                      po.verbose());

                // Output distances for the filtered graph.
                couplings.output_distances(po.out_fcg_filename(), graph_distances);
                if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output filtered graph distances to file " << po.out_fcg_filename()
                                            << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;
            }
        }

        if (po.paths_filename() != "") {
            // Read edge files for the single genome graphs.
            std::vector<std::string> path_edge_files;
            std::ifstream ifs(po.paths_filename());
            for (std::string path_edges; std::getline(ifs, path_edges); ) path_edge_files.emplace_back(path_edges);
            auto n_sggs = path_edge_files.size();
            
            Timer t_sgg, t_sgg_distances;
            int_t batch_size = po.concurrent_graphs(), n_nodes = 0, n_edges = 0;

            std::vector<SingleGenomeGraph> sg_graphs(batch_size);
            auto construct_sgg = [&combined_graph, &sg_graphs](int_t thr, const std::string& path_edges) { sg_graphs[thr] = SingleGenomeGraph(combined_graph, path_edges); };

            std::vector<distance_tuple_t> sgg_distances(po.n_couplings(), std::make_tuple(REAL_T_MAX, 0.0, 0.0, 0));

            for (auto i = 0; i < path_edge_files.size(); i += batch_size) {
                auto batch = std::min(i + batch_size, (int_t) path_edge_files.size()) - i;
                // Construct a batch of single genome graphs.
                if (po.verbose()) t_sgg.set_mark();
                std::vector<std::thread> threads;
                for (auto thr = 0; thr < batch; ++thr) threads.emplace_back(construct_sgg, thr, path_edge_files[i + thr]);
                for (auto& thr : threads) thr.join();
                if (po.verbose()) {
                    t_sgg.add_time_since_mark();
                    std::cout << timer.get_time_block_since_start() << " Created single genome graphs " << i + 1 << "-" << i + batch << " / " << n_sggs
                              << " in " << t_sgg.get_time_since_mark() << "." << std::endl;
                    for (const auto& sg_graph : sg_graphs) {
                        n_nodes += sg_graph.size();
                        for (const auto& node : sg_graph) n_edges += node.neighbors().size();
                    }
                    t_sgg_distances.set_mark();
                }

                // Calculate distances in the single genome graphs.
                for (const auto& sg_graph : sg_graphs) {
                    calculate_sgg_distances(sg_graph, search_jobs, sgg_distances, timer, couplings.size(), po.n_threads(), po.block_size(), po.max_distance());
                }
                if (po.verbose()) {
                    t_sgg_distances.add_time_since_mark();
                    std::cout << timer.get_time_block_since_start() << " Calculated distances for single genome graphs " << i + 1 << "-" << i + batch << " / " << n_sggs
                              << " in " << t_sgg_distances.get_time_since_mark() << "." << std::endl;
                }
            }
            if (po.verbose()) {
                n_nodes /= n_sggs;
                n_edges /= 2 * n_sggs;
                std::cout << timer.get_time_block_since_start() << " Creating " << n_sggs << " single genome graphs took " << t_sgg.get_stopwatch_time()
                          << ". The processed graphs have on average " << neat_number_str(n_nodes) << " connected nodes and " << neat_number_str(n_edges) << " edges." << std::endl;
                std::cout << timer.get_time_block_since_start_and_set_mark() << " Calculating distances for the " << n_sggs << " single genome graphs took "
                          << t_sgg_distances.get_stopwatch_time() << "." << std::endl;
            }

            // Output distances for the single genome graphs graph.
            std::vector<real_t> distances(sgg_distances.size());
            // Set distance correctly for disconnected couplings.
            for (auto& dist : sgg_distances) if (std::get<3>(dist) == 0) dist = std::make_tuple(REAL_T_MAX, REAL_T_MAX, REAL_T_MAX, 0);
            std::transform(sgg_distances.begin(), sgg_distances.end(), distances.begin(), [](const distance_tuple_t& dist) { return std::get<0>(dist); });
            couplings.output_distances(po.out_sgg_min_filename(), distances);
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output single genome graph min distances to file " << po.out_sgg_min_filename()
                                        << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;

            std::transform(sgg_distances.begin(), sgg_distances.end(), distances.begin(), [](const distance_tuple_t& dist) { return std::get<1>(dist); });
            couplings.output_distances(po.out_sgg_max_filename(), distances);
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output single genome graph max distances to file " << po.out_sgg_max_filename()
                                        << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;

            std::transform(sgg_distances.begin(), sgg_distances.end(), distances.begin(), [](const distance_tuple_t& dist) { return std::get<2>(dist); });
            couplings.output_distances(po.out_sgg_mean_filename(), distances);
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output single genome graph mean distances to file " << po.out_sgg_mean_filename()
                                        << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;

            std::vector<int_t> counts(sgg_distances.size());
            std::transform(sgg_distances.begin(), sgg_distances.end(), counts.begin(), [](const distance_tuple_t& dist) { return std::get<3>(dist); });
            couplings.output_counts(po.out_sgg_counts_filename(), counts);
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output connected coupling counts in the single genome graphs to file "
                                        << po.out_sgg_counts_filename() << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;
        }

        if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Finished." << std::endl;
    }
}

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Graph.hpp"
#include "GraphDistances.hpp"
#include "OperatingMode.hpp"
#include "Queries.hpp"
#include "ProgramOptions.hpp"
#include "SearchJobs.hpp"
#include "SingleGenomeGraph.hpp"
#include "SingleGenomeGraphDistances.hpp"
#include "Timer.hpp"
#include "types.hpp"

namespace unitig_distance {

    std::vector<std::string> get_fields(const std::string& line, char delim) {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        for (std::string field; std::getline(ss, field, delim); ) fields.push_back(std::move(field));
        return fields;
    }

    bool file_is_good(const std::string& filename) {
        return std::ifstream(filename).good();
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

    int_t fixed_distance(real_t distance, real_t max_distance) { return distance >= max_distance ? -1 : (int_t) distance; }

    int_t left_node(int_t v) { return v * 2; }
    int_t right_node(int_t v) { return v * 2 + 1; }

    bool is_numeric(const std::string& str) {
        double x;
        return (std::stringstream(str) >> x).eof();
    }

    template <typename T, int IDX>
    std::vector<T> transform_distance_tuple_vector(const std::vector<std::tuple<real_t, real_t, real_t, int_t>>& tuple_vector) {
        std::vector<T> vector(tuple_vector.size());
        static auto get_element = [](const std::tuple<real_t, real_t, real_t, int_t>& tuple) { return (T) std::get<IDX>(tuple); };
        std::transform(tuple_vector.begin(), tuple_vector.end(), vector.begin(), get_element);
        return vector;
    }
}

static bool sanity_check_input_files(const ProgramOptions& po) {
    if (!unitig_distance::file_is_good(po.edges_filename())) {
        std::cerr << "Can't open " << po.edges_filename() << std::endl;
        return false;
    }

    if (po.operating_mode(OperatingMode::FILTER)) {
        if (!unitig_distance::file_is_good(po.filter_filename())) {
            std::cerr << "Can't open " << po.filter_filename() << std::endl;
            return false;
        }
    }

    if (po.operating_mode(OperatingMode::CDBG)) {
        if (!unitig_distance::file_is_good(po.unitigs_filename())) {
            std::cerr << "Can't open " << po.unitigs_filename() << std::endl;
            return false;
        }

        if (po.operating_mode(OperatingMode::SGGS)) {
            if (!unitig_distance::file_is_good(po.sggs_filename())) {
                std::cerr << "Can't open " << po.sggs_filename() << std::endl;
                return false;
            }
            std::ifstream ifs(po.sggs_filename());
            for (std::string path_edges; std::getline(ifs, path_edges); ) {
                if (!unitig_distance::file_is_good(path_edges)) {
                    std::cerr << "Can't open " << path_edges << std::endl;
                    return false;
                }
            }
        }
    }

    if (!po.queries_filename().empty() && po.n_queries() > 0) {
        if (!unitig_distance::file_is_good(po.queries_filename())) {
            std::cerr << "Can't open " << po.queries_filename() << std::endl;
            return false;
        }
    }

    return true;
}

int main(int argc, char** argv) {
    Timer timer;

    std::cout << "unitig_distance | MIT License | Copyright (c) 2020-2022 Juri Kuronen\n\n";

    // Read command line arguments.
    ProgramOptions po(argc, argv);
    if (!po.valid_state() || !sanity_check_input_files(po)) return 1;

    if (po.verbose()) po.print_run_details();

    // Construct the graph according to operating mode.
    Graph graph;
    if (po.operating_mode(OperatingMode::GENERAL)) {
        graph = Graph(po.edges_filename(), po.graphs_one_based());
    } else { // OperatingMode::CDBG.
        graph = Graph(po.unitigs_filename(), po.edges_filename(), po.k(), po.graphs_one_based());
    }
    const std::string graph_name = po.operating_mode(OperatingMode::CDBG) ? "compacted de Bruijn graph" : "graph";

    if (graph.size() == 0) return 1; // Failed to construct the graph.

    if (po.verbose()) {
        std::cout << timer.get_time_block_since_start() << " Constructed " << graph_name << " in "
                  << timer.get_time_since_mark_and_set_mark() << ". ";
        graph.print_details();
    }

    // Construct the filtered graph if the filter file was provided.
    Graph filtered_graph;
    if (po.operating_mode(OperatingMode::FILTER)) {
        filtered_graph = Graph(graph, po.filter_filename(), po.filter_criterion());

        if (filtered_graph.size() == 0) return 1; // Failed to construct the filtered graph.

        if (po.verbose()) {
            std::cout << timer.get_time_block_since_start() << " Constructed filtered " << graph_name << " in "
                      << timer.get_time_since_mark_and_set_mark() << ". ";
            filtered_graph.print_details();
        }
    }

    // Read queries and calculate distances if the queries file was provided.
    if (!po.queries_filename().empty() && po.n_queries() > 0) {
        const Queries queries(po.queries_filename(), po.n_queries(), po.queries_one_based(), po.output_one_based(), po.max_distance());

        if (queries.size() == 0) return 1; // Failed to read queries.

        po.set_n_queries(queries.size());

        if (po.verbose()) {
            std::cout << timer.get_time_block_since_start() << " Read " << unitig_distance::neat_number_str(queries.size())
                      << " lines from queries file in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;
        }

        // Compute search jobs.
        const SearchJobs search_jobs(queries);

        if (po.verbose()) {
            std::cout << timer.get_time_block_since_start() << " Prepared " << unitig_distance::neat_number_str(search_jobs.size())
                      << " search jobs in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;
        }

        // Skip this part if user requested distances for the single genome graphs only.
        if (!(po.operating_mode(OperatingMode::SGGS) && po.run_sggs_only())) {
            if (po.verbose()) std::cout << timer.get_time_block_since_start_and_set_mark()
                                        << " Calculating distances in the " << graph_name << "." << std::endl;

            // Calculate distances.
            GraphDistances gd(graph, timer, po.n_threads(), po.block_size(), po.max_distance(), po.verbose());
            auto graph_distances = gd.solve(search_jobs);

            // Output distances.
            queries.output_distances(po.out_filename(), graph_distances);
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output " << graph_name << " distances to file "
                                        << po.out_filename() << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;
            
            if (filtered_graph.size() > 0) {
                if (po.verbose()) std::cout << timer.get_time_block_since_start_and_set_mark()
                                            << " Calculating distances in the filtered " << graph_name << "." << std::endl;

                // Calculate distances in the filtered graph.
                GraphDistances fgd(filtered_graph, timer, po.n_threads(), po.block_size(), po.max_distance(), po.verbose());
                auto filtered_graph_distances = fgd.solve(search_jobs);

                // Output filtered graph distances.
                queries.output_distances(po.out_filtered_filename(), filtered_graph_distances);
                if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output filtered " << graph_name << " distances to file "
                                            << po.out_filtered_filename() << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;
            }
        }

        // Calculate distances in the single genome graphs if the single genome graph files were provided.
        if (po.operating_mode(OperatingMode::SGGS)) {
            // Read single genome graph edge files.
            std::vector<std::string> path_edge_files;
            std::ifstream ifs(po.sggs_filename());
            for (std::string path_edges; std::getline(ifs, path_edges); ) path_edge_files.emplace_back(path_edges);
            auto n_sggs = path_edge_files.size();
            
            Timer t_sgg, t_sgg_distances;
            int_t batch_size = po.n_threads(), n_nodes = 0, n_edges = 0;

            std::vector<SingleGenomeGraph> sg_graphs(batch_size);
            auto construct_sgg = [&graph, &sg_graphs](int_t thr, const std::string& path_edges) {
                sg_graphs[thr] = SingleGenomeGraph(graph, path_edges);
            };

            std::vector<std::tuple<real_t, real_t, real_t, int_t>> sgg_distances(po.n_queries(), std::make_tuple(REAL_T_MAX, 0.0, 0.0, 0));

            for (std::size_t i = 0; i < path_edge_files.size(); i += batch_size) {
                // Construct a batch of single genome graphs.
                auto batch = std::min(i + batch_size, path_edge_files.size()) - i;

                if (po.verbose()) t_sgg.set_mark();

                std::vector<std::thread> threads;
                for (std::size_t thr = 0; thr < batch; ++thr) threads.emplace_back(construct_sgg, thr, path_edge_files[i + thr]);
                for (auto& thr : threads) thr.join();

                for (const auto& sg_graph : sg_graphs) if (sg_graph.size() == 0) return 1;

                if (po.verbose()) {
                    t_sgg.add_time_since_mark();
                    std::cout << timer.get_time_block_since_start() << " Constructed single genome graphs " << i + 1 << "-" << i + batch << " / " << n_sggs
                              << " in " << t_sgg.get_time_since_mark() << "." << std::endl;
                    // Update n_nodes and n_edges.
                    for (const auto& sg_graph : sg_graphs) {
                        n_nodes += sg_graph.size();
                        for (const auto& adj : sg_graph) n_edges += adj.size();
                    }
                    t_sgg_distances.set_mark();
                }

                // Calculate distances in the single genome graphs.
                for (const auto& sg_graph : sg_graphs) {
                    SingleGenomeGraphDistances sggd(sg_graph, sgg_distances, po.n_threads(), po.block_size(), po.max_distance(), po.verbose());
                    sggd.solve(search_jobs); // Updates sgg_distances.
                }
                if (po.verbose()) {
                    t_sgg_distances.add_time_since_mark();
                    std::cout << timer.get_time_block_since_start() << " Calculated distances in the single genome graphs " << i + 1 << "-" << i + batch
                              << " / " << n_sggs << " in " << t_sgg_distances.get_time_since_mark() << "." << std::endl;
                }
            }

            if (po.verbose()) {
                n_nodes /= n_sggs;
                n_edges /= 2 * n_sggs;
                std::cout << timer.get_time_block_since_start() << " Constructing " << n_sggs << " single genome graphs took " << t_sgg.get_stopwatch_time()
                          << ". The compressed single genome graphs have on average " << unitig_distance::neat_number_str(n_nodes) << " connected nodes and "
                          << unitig_distance::neat_number_str(n_edges) << " edges." << std::endl;
                std::cout << timer.get_time_block_since_start_and_set_mark() << " Calculating distances in the " << n_sggs << " single genome graphs took "
                          << t_sgg_distances.get_stopwatch_time() << "." << std::endl;
            }

            // Set distance correctly for disconnected queries.
            for (auto& distance : sgg_distances) {
                if (std::get<3>(distance) == 0) distance = std::make_tuple(REAL_T_MAX, REAL_T_MAX, REAL_T_MAX, 0);
            }

            // Output single genome graphs graph distances.
            queries.output_distances(po.out_sgg_min_filename(), unitig_distance::transform_distance_tuple_vector<real_t, 0>(sgg_distances));
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output single genome graph min distances to file "
                                        << po.out_sgg_min_filename() << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;

            queries.output_distances(po.out_sgg_max_filename(), unitig_distance::transform_distance_tuple_vector<real_t, 1>(sgg_distances));
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output single genome graph max distances to file "
                                        << po.out_sgg_max_filename() << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;

            queries.output_distances(po.out_sgg_mean_filename(), unitig_distance::transform_distance_tuple_vector<real_t, 2>(sgg_distances));
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output single genome graph mean distances to file "
                                        << po.out_sgg_mean_filename() << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;

            queries.output_counts(po.out_sgg_counts_filename(), unitig_distance::transform_distance_tuple_vector<int_t, 3>(sgg_distances));
            if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Output single genome graph connected vertex pair counts to file "
                                        << po.out_sgg_counts_filename() << " in " << timer.get_time_since_mark_and_set_mark() << "." << std::endl;
        }

        if (po.verbose()) std::cout << timer.get_time_block_since_start() << " Finished." << std::endl;
    }
}

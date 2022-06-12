#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "DistanceVector.hpp"
#include "QueriesReader.hpp"
#include "Graph.hpp"
#include "GraphBuilder.hpp"
#include "GraphDistances.hpp"
#include "OperatingMode.hpp"
#include "OutlierTools.hpp"
#include "PrintUtils.hpp"
#include "ProgramOptions.hpp"
#include "Queries.hpp"
#include "ResultsWriter.hpp"
#include "SearchJobs.hpp"
#include "SingleGenomeGraph.hpp"
#include "SingleGenomeGraphBuilder.hpp"
#include "SingleGenomeGraphDistances.hpp"
#include "Timer.hpp"
#include "types.hpp"
#include "Utils.hpp"

using po = ProgramOptions;

static int fail_with_error(const std::string& error) { std::cerr << error << std::endl; return 1; }

int main(int argc, char** argv) {
    Timer timer;

    // Read command line arguments.
    po::read_command_line_arguments(argc, argv);
    if (po::verbose) PrintUtils::print_license();
    if (!po::valid_state || !Utils::sanity_check_input_files()) return 1;
    if (po::verbose) po::print_run_details();

    // Read queries.
    const Queries queries = QueriesReader::read_queries();
    if (queries.size() == 0) return fail_with_error("Failed to read queries.");
    if (po::verbose) PrintUtils::print_tbss_tsmasm(timer, "Read", Utils::neat_number_str(queries.size()), "lines from queries file");

    // Set up outlier tools.
    OutlierTools ot(queries, timer);

    // Operating in outliers tool mode only.
    if (po::operating_mode == OperatingMode::OUTLIER_TOOLS) {
        ot.determine_and_output_outliers(queries.distances(), po::out_outliers_filename(), po::out_outlier_stats_filename());
        return 0;
    }

    // Compute search jobs.
    const SearchJobs search_jobs(queries);
    if (po::verbose) PrintUtils::print_tbss_tsmasm(timer, "Prepared", Utils::neat_number_str(search_jobs.size()), "search jobs");

    // Construct the graph according to operating mode.
    Graph graph = GraphBuilder::build_correct_graph();
    if (graph.size() == 0) return fail_with_error("Failed to construct main graph.");
    if (po::verbose) {
        PrintUtils::print_tbss_tsmasm_noendl(timer, "Constructed main graph");
        graph.print_details();
    }

    // Start with single genome graphs.
    DistanceVector sgg_distances(queries.size(), 0.0, 0);

    // Calculate distances in the single genome graphs if the single genome graph files were provided.
    if (po::has_operating_mode(OperatingMode::SGGS)) {
        // Read single genome graph edge files.
        std::vector<std::string> path_edge_files;
        std::ifstream ifs(po::sggs_filename);
        for (std::string path_edges; std::getline(ifs, path_edges); ) path_edge_files.emplace_back(path_edges);
        std::size_t n_sggs = path_edge_files.size(), batch_size = po::n_threads;

        if (n_sggs == 0) fail_with_error("Couldn't read single genome graph files.");

        // Printing variables for verbose mode.
        Timer t_sgg, t_sgg_distances, t_deconstruct;
        int_t print_interval = (n_sggs + 4) / 5, print_i = 1, n_nodes = 0, n_edges = 0;
        if (print_interval % batch_size) print_interval += batch_size - (print_interval % batch_size); // Round up.
        bool print_now = false;

        if (po::verbose) PrintUtils::print_tbssasm(timer, "Calculating distances in the single genome graphs");

        for (std::size_t i = 0; i < n_sggs; i += batch_size) {
            if (po::verbose) {
                t_deconstruct.add_time_since_mark();
                if (print_now) {
                    auto stslasl = t_deconstruct.get_stopwatch_time_since_lap_and_set_lap();
                    PrintUtils::print_tbss(timer, "Deconstructed single genome graphs", print_i, "-", i, "/", n_sggs, "in", stslasl);
                    print_i = i + 1;
                }
                t_sgg.set_mark();
            }

            if (po::verbose) print_now = (i + batch_size) % print_interval == 0 || (i + batch_size) >= n_sggs;

            auto batch = std::min(i + batch_size, n_sggs) - i;

            // Construct a batch of single genome graphs.
            std::vector<SingleGenomeGraph> sg_graphs(batch);
            auto construct_sgg = [&graph, &sg_graphs](int_t thr, const std::string& path_edges) {
                sg_graphs[thr] = SingleGenomeGraphBuilder::build_sgg(graph, path_edges);
            };

            std::vector<std::thread> threads;
            for (std::size_t thr = 0; thr < batch; ++thr) threads.emplace_back(construct_sgg, thr, path_edge_files[i + thr]);
            for (auto& thr : threads) thr.join();

            for (const auto& sg_graph : sg_graphs) {
                if (sg_graph.size() == 0) fail_with_error("Failed to construct single genome graph.");
            }

            if (po::verbose) {
                t_sgg.add_time_since_mark();
                if (print_now) {
                    auto stslasl = t_sgg.get_stopwatch_time_since_lap_and_set_lap();
                    PrintUtils::print_tbss(timer, "Constructed single genome graphs", print_i, "-", i + batch, "/", n_sggs, "in", stslasl);
                }
                // Update n_nodes and n_edges.
                for (const auto& sg_graph : sg_graphs) {
                    n_nodes += sg_graph.size();
                    for (const auto& adj : sg_graph) n_edges += adj.size();
                }
                t_sgg_distances.set_mark();
            }

            // Calculate distances in the single genome graphs.
            for (const auto& sg_graph : sg_graphs) {
                auto sgg_batch_distances = SingleGenomeGraphDistances(sg_graph).solve(search_jobs);
                // Combine results across threads.
                for (const auto& distances : sgg_batch_distances) {
                    for (const auto& result : distances) {
                        int_t original_idx;
                        Distance distance;
                        std::tie(original_idx, distance) = result;
                        sgg_distances[original_idx] += distance;
                    }
                }
            }

            if (po::verbose) {
                t_sgg_distances.add_time_since_mark();
                if (print_now) {
                    auto stslasl = t_sgg_distances.get_stopwatch_time_since_lap_and_set_lap();
                    PrintUtils::print_tbss(timer, "Calculated distances in the single genome graphs", print_i, "-", i + batch, "/", n_sggs, "in", stslasl);
                }
                t_deconstruct.set_mark();
            }
        }

        if (po::verbose) {
            t_deconstruct.add_time_since_mark();
            auto stslasl = t_deconstruct.get_stopwatch_time_since_lap_and_set_lap();
            PrintUtils::print_tbss(timer, "Deconstructed single genome graphs and distances", print_i, "-", n_sggs, "/", n_sggs, "in", stslasl);
        }

        // Set distance correctly for disconnected queries.
        for (auto& distance : sgg_distances) if (distance.count() == 0) distance = Distance(REAL_T_MAX, 0);

        if (po::verbose) {
            n_nodes /= n_sggs;
            n_edges /= 2 * n_sggs;
            PrintUtils::print_tbss(timer, "Constructing", n_sggs, "single genome graphs took", t_sgg.get_stopwatch_time());
            PrintUtils::print_tbss(timer, "The compressed single genome graphs have on average", Utils::neat_number_str(n_nodes), "connected nodes and", 
                                   Utils::neat_number_str(n_edges), "edges");
            PrintUtils::print_tbss(timer, "Calculating distances in the", n_sggs, "single genome graphs took", t_sgg_distances.get_stopwatch_time());
            PrintUtils::print_tbssasm(timer, "Deconstructing", n_sggs, "single genome graphs took", t_deconstruct.get_stopwatch_time());
        }

        // Output single genome graphs graph distances.
        ResultsWriter::output_results(po::out_sgg_filename(), queries, sgg_distances);
        if (po::verbose) PrintUtils::print_tbss_tsmasm(timer, "Output single genome graph mean distances to file", po::out_sgg_filename());

        // Determine outliers.
        if (po::has_operating_mode(OperatingMode::OUTLIER_TOOLS)) {
            ot.determine_and_output_outliers(sgg_distances, po::out_sgg_outliers_filename(), po::out_sgg_outlier_stats_filename());
        }

    }

    // Run normal graph
    if (!po::run_sggs_only) {
        if (po::verbose) PrintUtils::print_tbssasm(timer, "Calculating distances in the main graph");

        // Calculate distances.
        auto graph_distances = GraphDistances(graph, timer).solve(search_jobs);
        timer.set_mark();

        ResultsWriter::output_results(po::out_filename(), queries, graph_distances);
        if (po::verbose) PrintUtils::print_tbss_tsmasm(timer, "Output main graph distances to file", po::out_filename());

        // Determine outliers.
        if (po::has_operating_mode(OperatingMode::OUTLIER_TOOLS)) {
            ot.determine_and_output_outliers(graph_distances, po::out_outliers_filename(), po::out_outlier_stats_filename());
        }

    }

    if (po::verbose) PrintUtils::print_tbss(timer, "Finished");
}

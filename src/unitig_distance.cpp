#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Distance.hpp"
#include "DistanceVector.hpp"
#include "Graph.hpp"
#include "GraphBuilder.hpp"
#include "GraphDistances.hpp"
#include "OperatingMode.hpp"
#include "OutlierTools.hpp"
#include "PrintUtils.hpp"
#include "ProgramOptions.hpp"
#include "Queries.hpp"
#include "SearchJobs.hpp"
#include "SingleGenomeGraph.hpp"
#include "SingleGenomeGraphBuilder.hpp"
#include "SingleGenomeGraphDistances.hpp"
#include "Timer.hpp"
#include "types.hpp"
#include "Utils.hpp"

static void determine_outliers(const Queries& queries,
                               const std::vector<real_t>& distances,
                               const std::vector<int_t>& counts,
                               const ProgramOptions& po,
                               const std::string& graph_name,
                               const std::string& out_outliers_filename,
                               const std::string& out_outlier_stats_filename,
                               Timer& timer)
{
    timer.set_mark();
    OutlierTools ot(queries, distances, counts, po.sgg_count_threshold(), po.max_distance(), po.verbose());
    if (po.ld_distance() >= 0 && po.outlier_threshold() >= 0.0) {
        // Use custom values.
        ot.determine_outliers(po.ld_distance(), po.outlier_threshold());
    } else {
        // Estimate outlier thresholds. Also estimate linkage disequilibrium distance if ld_distance < 0.
        ot.determine_outliers(po.ld_distance(), po.ld_distance_nth_score(), po.ld_distance_min(), po.ld_distance_score());
    }
    if (po.verbose()) {
        if (ot.ok()) {
            PrintUtils::print_tbss_tsmasm(timer, "Determined outliers for", graph_name, "distances");
        } else {
            PrintUtils::print_tbss(timer, "Unable to determine outliers for", graph_name, "distances:", ot.reason());
        }
        ot.print_details();
    }
    if (!ot.ok()) return;
    ot.output_outliers(out_outliers_filename, out_outlier_stats_filename);
    if (po.verbose()) PrintUtils::print_tbss_tsmasm(timer, "Output outliers to files", out_outliers_filename, "and", out_outlier_stats_filename);
}

static std::vector<int_t> read_counts(const std::string& counts_filename, int_t n_queries) {
    std::vector<int_t> counts;
    std::ifstream ifs(counts_filename);
    int_t cnt = 0;
    for (std::string line; std::getline(ifs, line); ) {
        auto fields = Utils::get_fields(line);
        if (fields.size() < 3) {
            std::cout << "Wrong number of fields in counts file: " << counts_filename << std::endl;
            return std::vector<int_t>();
        }
        counts.push_back(std::stoll(fields[2]));
        if (++cnt == n_queries) break;
    }

    // Did not manage to read enough counts.
    if (cnt != n_queries) {
        std::cout << "Could not read enough counts (" << cnt << "!=" << n_queries << "), counts will be omitted." << std::endl;
        return std::vector<int_t>();
    }

    return counts;
}

int main(int argc, char** argv) {
    Timer timer;

    // Read command line arguments.
    ProgramOptions po(argc, argv);
    if (po.verbose()) PrintUtils::print_license();
    if (!po.valid_state() || !Utils::sanity_check_input_files(po)) return 1;
    if (po.verbose()) po.print_run_details();

    // Operating in outliers tool mode only.
    if (po.operating_mode() == OperatingMode::OUTLIER_TOOLS) {
        if (po.n_queries() == 0) po.set_n_queries(INT_T_MAX);

        const Queries queries(po.queries_filename(), po.n_queries(), true, po.queries_one_based());
        if (!queries.using_extended_queries()) {
            std::cout << "Queries missing required fields." << std::endl;
            return 1;
        }
        if (po.verbose()) {
            PrintUtils::print_tbss_tsmasm(timer, "Read", Utils::neat_number_str(queries.size()), "lines from queries file");
        }

        po.set_n_queries(queries.size());

        const auto distances = queries.get_distance_vector();
        if (po.verbose()) PrintUtils::print_tbss_tsmasm(timer, "Got distance vector");

        std::vector<int_t> counts;
        if (!po.sgg_counts_filename().empty()) {
            counts = read_counts(po.sgg_counts_filename(), po.n_queries());
            if (counts.empty()) return 1;
            if (po.verbose()) PrintUtils::print_tbss_tsmasm(timer, "Read", Utils::neat_number_str(counts.size()), "counts");
        }

        determine_outliers(queries, distances, counts, po, "provided",
                           po.out_outliers_filename(), po.out_outlier_stats_filename(), timer);
        return 0;
    }

    // Construct the graph according to operating mode.
    Graph graph = GraphBuilder::build_correct_graph(po);

    const std::string graph_name = po.operating_mode(OperatingMode::CDBG) ? "compacted de Bruijn graph" : "graph";

    if (graph.size() == 0) {
        std::cout << "Failed to construct " << graph_name << "." << std::endl;
        return 1;
    }

    if (po.verbose()) {
        PrintUtils::print_tbss_tsmasm_noendl(timer, "Constructed", graph_name);
        graph.print_details();
    }

    // Read queries and calculate distances if the queries file was provided.
    if (!po.queries_filename().empty() && po.n_queries() > 0) {
        const Queries queries(po.queries_filename(), po.n_queries(), po.operating_mode(OperatingMode::OUTLIER_TOOLS),
                              po.queries_one_based(), po.output_one_based(), po.max_distance());

        if (queries.size() == 0) {
            std::cout << "Failed to read queries." << std::endl;
            return 1;
        }

        po.set_n_queries(queries.size());

        if (po.verbose()) PrintUtils::print_tbss_tsmasm(timer, "Read", Utils::neat_number_str(queries.size()), "lines from queries file");

        // Compute search jobs.
        const SearchJobs search_jobs(queries);

        if (po.verbose()) PrintUtils::print_tbss_tsmasm(timer, "Prepared", Utils::neat_number_str(search_jobs.size()), "search jobs");

        std::vector<real_t> graph_distances;

        // Skip this part if user requested distances for the single genome graphs only.
        if (!(po.operating_mode(OperatingMode::SGGS) && po.run_sggs_only())) {
            if (po.verbose()) PrintUtils::print_tbssasm(timer, "Calculating distances in the", graph_name);

            // Calculate distances.
            GraphDistances gd(graph, timer, po.n_threads(), po.block_size(), po.max_distance(), po.verbose());
            graph_distances = gd.solve(search_jobs);

            // Output distances.
            queries.output_distances(po.out_filename(), graph_distances);
            if (po.verbose()) PrintUtils::print_tbss_tsmasm(timer, "Output", graph_name, "distances to file", po.out_filename());

            // Determine outliers.
            if (po.operating_mode(OperatingMode::OUTLIER_TOOLS)) {
                if (po.operating_mode(OperatingMode::SGGS) && po.sgg_count_threshold() > 0) {
                    if (po.verbose()) PrintUtils::print_tbssasm(timer, "Running in SGGS mode with sgg_count_threshold > 0. Will determine outliers for",
                                                                graph_name, "distances after the single genome graph routines");
                } else {
                    determine_outliers(queries, graph_distances, std::vector<int_t>(), po, graph_name,
                                       po.out_outliers_filename(), po.out_outlier_stats_filename(), timer);
                }
            }
        }

        // Calculate distances in the single genome graphs if the single genome graph files were provided.
        if (po.operating_mode(OperatingMode::SGGS)) {
            // Read single genome graph edge files.
            std::vector<std::string> path_edge_files;
            std::ifstream ifs(po.sggs_filename());
            for (std::string path_edges; std::getline(ifs, path_edges); ) path_edge_files.emplace_back(path_edges);
            std::size_t n_sggs = path_edge_files.size(), batch_size = po.n_sggs_to_hold_in_memory();

            if (n_sggs == 0) {
                std::cout << "Couldn't read single genome graph files." << std::endl;
                return 1;
            }

            // Printing variables for verbose mode.
            Timer t_sgg, t_sgg_distances, t_deconstruct;
            int_t print_interval = (n_sggs + 4) / 5, print_i = 1, n_nodes = 0, n_edges = 0;
            if (print_interval % batch_size) print_interval += batch_size - (print_interval % batch_size); // Round up.
            bool print_now = false;

            DistanceVector sgg_distances(po.n_queries(), 0.0, 0);

            if (po.verbose()) PrintUtils::print_tbssasm(timer, "Calculating distances in the single genome graphs");

            for (std::size_t i = 0; i < n_sggs; i += batch_size) {
                if (po.verbose()) {
                    t_deconstruct.add_time_since_mark();
                    if (print_now) {
                        auto stslasl = t_deconstruct.get_stopwatch_time_since_lap_and_set_lap();
                        PrintUtils::print_tbss(timer, "Deconstructed single genome graphs", print_i, "-", i, "/", n_sggs, "in", stslasl);
                        print_i = i + 1;
                    }
                    t_sgg.set_mark();
                }

                if (po.verbose()) print_now = (i + batch_size) % print_interval == 0 || (i + batch_size) >= n_sggs;

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
                    if (sg_graph.size() == 0) {
                        std::cout << "Failed to construct single genome graph." << std::endl;
                        return 1;
                    }
                }

                if (po.verbose()) {
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
                    SingleGenomeGraphDistances sggd(sg_graph, po.n_threads(), po.block_size(), po.max_distance());
                    auto sgg_batch_distances = sggd.solve(search_jobs);
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

                if (po.verbose()) {
                    t_sgg_distances.add_time_since_mark();
                    if (print_now) {
                        auto stslasl = t_sgg_distances.get_stopwatch_time_since_lap_and_set_lap();
                        PrintUtils::print_tbss(timer, "Calculated distances in the single genome graphs", print_i, "-", i + batch, "/", n_sggs, "in", stslasl);
                    }
                    t_deconstruct.set_mark();
                }
            }

            if (po.verbose()) {
                t_deconstruct.add_time_since_mark();
                auto stslasl = t_deconstruct.get_stopwatch_time_since_lap_and_set_lap();
                PrintUtils::print_tbss(timer, "Deconstructed single genome graphs and distances", print_i, "-", n_sggs, "/", n_sggs, "in", stslasl);
            }

            // Set distance correctly for disconnected queries.
            for (auto& distance : sgg_distances) if (distance.count() == 0) distance = Distance(REAL_T_MAX, 0);

            if (po.verbose()) {
                n_nodes /= n_sggs;
                n_edges /= 2 * n_sggs;
                PrintUtils::print_tbss(timer, "Constructing", n_sggs, "single genome graphs took", t_sgg.get_stopwatch_time());
                PrintUtils::print_tbss(timer, "The compressed single genome graphs have on average", Utils::neat_number_str(n_nodes), "connected nodes and", 
                                       Utils::neat_number_str(n_edges), "edges");
                PrintUtils::print_tbss(timer, "Calculating distances in the", n_sggs, "single genome graphs took", t_sgg_distances.get_stopwatch_time());
                PrintUtils::print_tbssasm(timer, "Deconstructing", n_sggs, "single genome graphs took", t_deconstruct.get_stopwatch_time());
            }

            // Output single genome graphs graph distances.
            auto mean_distances = sgg_distances.distances();
            queries.output_distances(po.out_sgg_mean_filename(), mean_distances);
            if (po.verbose()) PrintUtils::print_tbss_tsmasm(timer, "Output single genome graph mean distances to file", po.out_sgg_mean_filename());

            auto sgg_counts = sgg_distances.counts();
            queries.output_counts(po.out_sgg_counts_filename(), sgg_counts);
            if (po.verbose()) PrintUtils::print_tbss_tsmasm(timer, "Output single genome graph connected vertex pair counts to file", po.out_sgg_counts_filename());

            // Determine outliers.
            if (po.operating_mode(OperatingMode::OUTLIER_TOOLS)) {
                // Determine outliers for the full graphs here if using sgg count filtering.
                if (!po.run_sggs_only() && po.sgg_count_threshold() > 0) {
                    if (graph_distances.size() > 0) {
                        determine_outliers(queries, graph_distances, sgg_counts, po, graph_name,
                                           po.out_outliers_filename(), po.out_outlier_stats_filename(), timer);
                    }
                }

                // Determine outliers.
                if (po.operating_mode(OperatingMode::OUTLIER_TOOLS)) {
                    determine_outliers(queries, mean_distances, sgg_counts, po, "single genome graph mean distances",
                                       po.out_sgg_mean_outliers_filename(), po.out_sgg_mean_outlier_stats_filename(), timer);
                }
            }
        }

        if (po.verbose()) PrintUtils::print_tbss(timer, "Finished");
    }
}

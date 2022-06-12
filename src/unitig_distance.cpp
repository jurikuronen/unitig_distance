#include <iostream>
#include <string>

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

    // Calculate distances in the single genome graphs if the single genome graph files were provided.
    if (po::has_operating_mode(OperatingMode::SGGS)) {
        auto sgg_distances = calculate_sgg_distances(graph, search_jobs, timer);

        if (sgg_distances.size() == 0) return 1;

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

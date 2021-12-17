#pragma once

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "OperatingMode.hpp"
#include "types.hpp"

// A command line argument handler class.
class ProgramOptions {
public:
    ProgramOptions(int argc, char** argv) : m_argc(argc), m_argv(argv) {
        if (has_arg("-h", "--help")) {
            print_help();
            m_valid_state = false;
            return;
        }
        set_value(m_unitigs_filename, "-U", "--unitigs-file");
        set_value(m_edges_filename, "-E", "--edges-file");
        set_value(m_queries_filename, "-Q", "--queries-file");
        set_value(m_sggs_filename, "-S", "--sgg-paths-file");
        set_value(m_out_stem, "-o", "--output-stem");
        set_value(m_filter_filename, "-F", "--filter-file");
        set_value(m_k, "-k", "--k-mer-length");
        set_value(m_n_queries, "-n", "--n-queries");
        set_value(m_block_size, "-b", "--block-size");
        set_value(m_max_distance, "-d", "--max-distance");
        set_value(m_n_threads, "-t", "--threads");
        set_value(m_filter_criterion, "-c", "--filter-criterion");
        m_graphs_one_based = has_arg("-1g", "--graphs-one-based");
        m_queries_one_based = has_arg("-1q", "--queries-one-based");
        m_output_one_based = has_arg("-1o", "--output-one-based");
        m_run_sggs_only = has_arg("-r", "--run-sggs-only");
        m_verbose = has_arg("-v", "--verbose");

        set_operating_mode();

        m_valid_state = all_required_arguments_provided();
    }

    const std::string& unitigs_filename() const { return m_unitigs_filename; }
    const std::string& edges_filename() const { return m_edges_filename; }
    const std::string& queries_filename() const { return m_queries_filename; }
    const std::string& sggs_filename() const { return m_sggs_filename; }
    const std::string& out_stem() const { return m_out_stem; }
    const std::string& filter_filename() const { return m_filter_filename; }
    int_t k() const { return m_k; }
    int_t n_queries() const { return m_n_queries; }
    int_t block_size() const { return m_block_size; }
    real_t max_distance() const { return m_max_distance; }
    int_t n_threads() const { return m_n_threads; }
    real_t filter_criterion() const { return m_filter_criterion; }
    bool graphs_one_based() const { return m_graphs_one_based; }
    bool queries_one_based() const { return m_queries_one_based; }
    bool output_one_based() const { return m_output_one_based; }
    bool run_sggs_only() const { return m_run_sggs_only; }
    bool verbose() const { return m_verbose; }

    bool valid_state() const { return m_valid_state; }

    const OperatingMode& operating_mode() const { return m_om; }
    bool operating_mode(const OperatingMode& om) const { return operating_mode_to_bool(m_om & om); }

    const std::string out_filename() const { return out_stem() + ".ud" + based_str(); }
    const std::string out_filtered_filename() const { return out_stem() + ".ud_filtered" + based_str(); }
    const std::string out_sgg_min_filename() const { return out_stem() + ".ud_sgg_min" + based_str(); }
    const std::string out_sgg_max_filename() const { return out_stem() + ".ud_sgg_max" + based_str(); }
    const std::string out_sgg_mean_filename() const { return out_stem() + ".ud_sgg_mean" + based_str(); }
    const std::string out_sgg_counts_filename() const { return out_stem() + ".ud_sgg_counts" + based_str(); }

    // For updating the value after queries were read.
    void set_n_queries(int_t n_queries) { m_n_queries = n_queries; }

    // Print details about this run.
    void print_run_details() const {
        std::vector<std::string> arguments;

        double_push_back(arguments, "  --edges-file", edges_filename());
        double_push_back(arguments, "  --graphs-one-based", graphs_one_based() ? "TRUE" : "FALSE");
        if (operating_mode(OperatingMode::CDBG)) {
            double_push_back(arguments, "  --unitigs-file", unitigs_filename());
            double_push_back(arguments, "  --k-mer-length", std::to_string(k()));
        }
        if (operating_mode(OperatingMode::FILTER)) {
            double_push_back(arguments, "  --filter-file", filter_filename());
            double_push_back(arguments, "  --filter-criterion", std::to_string(filter_criterion()));
        }
        if (operating_mode(OperatingMode::SGGS)) {
            double_push_back(arguments, "  --sgg-paths-file", sggs_filename());
            double_push_back(arguments, "  --run-sggs-only", run_sggs_only() ? "TRUE" : "FALSE");
        }
        if (!queries_filename().empty() && n_queries() > 0) {
            double_push_back(arguments, "  --queries-file", queries_filename());
            double_push_back(arguments, "  --n-queries", n_queries() == INT_T_MAX ? "ALL" : std::to_string(n_queries()));
            double_push_back(arguments, "  --queries-one-based", queries_one_based() ? "TRUE" : "FALSE");
            double_push_back(arguments, "  --block-size", std::to_string(block_size()));
            double_push_back(arguments, "  --max-distance", max_distance() == REAL_T_MAX ? "INF" : std::to_string(max_distance()));
        }
        double_push_back(arguments, "  --output-stem", out_stem());
        double_push_back(arguments, "  --output-one-based", output_one_based() ? "TRUE" : "FALSE");
        double_push_back(arguments, "  --threads", std::to_string(n_threads()));

        std::cout << "Using following arguments:" << std::endl;
        for (std::size_t i = 0; i < arguments.size(); i += 2) std::printf("%-30s %s\n", arguments[i].data(), arguments[i + 1].data());
        std::cout << std::endl;

        std::cout << "Operating mode: " << operating_mode() << std::endl;
    }

private:
    int m_argc;
    char** m_argv;

    std::string m_unitigs_filename = "";
    std::string m_edges_filename = "";
    std::string m_queries_filename = "";
    std::string m_sggs_filename = "";
    std::string m_out_stem = "out";
    std::string m_filter_filename = "";
    int_t m_k = 0;
    int_t m_n_queries = INT_T_MAX;
    int_t m_block_size = 50000;
    real_t m_max_distance = REAL_T_MAX;
    int_t m_n_threads = 1;
    real_t m_filter_criterion = 2.0;
    bool m_graphs_one_based = false;
    bool m_queries_one_based = false;
    bool m_output_one_based = false;
    bool m_run_sggs_only = false;
    bool m_verbose = false;

    bool m_valid_state;

    OperatingMode m_om = OperatingMode::DEFAULT;

    void set_operating_mode() {
        if (!filter_filename().empty()) m_om |= OperatingMode::FILTER;
        if (unitigs_filename().empty()) {
            m_om |= OperatingMode::GENERAL;
        } else {
            m_om |= OperatingMode::CDBG;
            if (!sggs_filename().empty()) m_om |= OperatingMode::SGGS;
        }
    }

    const std::string based_str() const { return output_one_based() ? "_1_based" : "_0_based"; }

    char** begin() const { return m_argv; }
    char** end() const { return m_argv + m_argc; }
    char** find(const std::string& opt) const { return std::find(begin(), end(), opt); }
    char* has_arg(const std::string& opt) const { return has_arg(opt, opt); }
    char* has_arg(const std::string& opt, const std::string& alt) const {
        // Don't need the value, return either nullptr or anything for a boolean check.
        return find(opt) != end() || find(alt) != end() ? begin()[0] : nullptr;
    }
    char* find_arg_value(const std::string& opt) const { return find_arg_value(opt, opt); }
    char* find_arg_value(const std::string& opt, const std::string& alt) const {
        auto it = find(opt);
        if (it != end() && ++it != end()) return *it;
        it = find(alt);
        return it != end() && ++it != end() ? *it : nullptr;
    }

    bool all_required_arguments_provided() const {
        bool ok = true;
        if (edges_filename().empty()) std::cout << "Missing edges filename.\n", ok = false;
        if (operating_mode(OperatingMode::CDBG) && k() <= 0) std::cout << "Missing k-mer length.\n", ok = false;
        if (!ok) print_no_args();
        return ok;
    }

    void print_no_args() const { std::cout << "Use '-h' or '--help' for a list of available options.\n"; }

    void print_help() const {
        std::vector<std::string> options{
            "Graph edges (required always):", "",
            "  -E [ --edges-file ] arg", "Path to file containing graph edges.",
            "  -1g [ --graphs-one-based ]", "Graph files use one-based numbering.",
            "", "",
            "Filter the graph:", "",
            "  -F [ --filter-file ] arg", "Path to file containing vertices/unitigs that will be filtered.",
            "  -c [ --filter-criterion ] arg (=2.0)", "Criterion for the filter.",
            "", "",
            "CDBG operating mode:", "",
            "  -U [ --unitigs-file ] arg", "Path to file containing unitigs.",
            "  -k [ --k-mer-length ] arg", "k-mer length.",
            "", "",
            "CDBG and/or SGGS operating mode:", "",
            "  -S [ --sgg-paths-file ] arg", "Path to file containing paths to single genome graph edge files.",
            "  -r [ --run-sggs-only ]", "Calculate distances only in the single genome graphs.",
            "", "",
            "Distance queries:", "",
            "  -Q [ --queries-file ] arg", "Path to queries file.",
            "  -n [ --n-queries ] arg (=inf)", "Number of queries to read from the queries file.",
            "  -1q [ --queries-one-based ]", "Queries file uses one-based numbering.",
            "  -b [ --block-size ] arg (=50000)", "Process this many queries/tasks at a time.",
            "  -d [ --max-distance ] arg (=inf)", "Maximum allowed graph distance (for constraining the searches).",
            "", "",
            "Optional arguments.", "",
            "  -o [ --output-stem ] arg (=out)", "Path for output files (without extension).",
            "  -1o [ --output-one-based ]", "Output files use one-based numbering.",
            "  -t [ --threads ] arg (=1)", "Number of threads.",
            "  -v [ --verbose ]", "Be verbose.",
            "  -h [ --help ]", "Print this list.",
        };
        for (std::size_t i = 0; i < options.size(); i += 2) std::printf("%-45s %s\n", options[i].data(), options[i + 1].data());
    }

    void double_push_back(std::vector<std::string>& arguments, const std::string& opt, const std::string& val) const {
        arguments.push_back(opt);
        arguments.push_back(val);
    }

    template <typename T>
    void set_value(T& value, const std::string& opt, const std::string& alt) { if (has_arg(opt, alt)) std::stringstream(find_arg_value(opt, alt)) >> value; }

};


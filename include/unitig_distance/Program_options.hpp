#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "types.hpp"

// A command line argument handler class.
class Program_options {
public:
    Program_options(int argc, char** argv) {
        char** argv_end = argv + argc;
        char* arg_reader = find_arg_name(argv, argv_end, "-h", "--help");
        if (arg_reader) {
            print_help();
            m_valid_state = false;
            return;
        }
        // Parse required arguments.
        if (arg_reader = find_arg_value(argv, argv_end, "-V", "--vertices-file")) m_nodes_filename = std::string(arg_reader);
        if (arg_reader = find_arg_value(argv, argv_end, "-E", "--edges-file")) m_edges_filename = std::string(arg_reader);
        if (arg_reader = find_arg_value(argv, argv_end, "-k", "--k-mer-length")) m_k = std::stoll(arg_reader);
        // Required if --n_couplings > 0.
        if (arg_reader = find_arg_value(argv, argv_end, "-C", "--couplings-file")) m_couplings_filename = std::string(arg_reader);
        // Parse optional arguments.
        if (arg_reader = find_arg_value(argv, argv_end, "-P", "--paths-file")) m_paths_filename = std::string(arg_reader);
        if (arg_reader = find_arg_value(argv, argv_end, "-o", "--output-stem")) m_out_stem = std::string(arg_reader);
        if (arg_reader = find_arg_value(argv, argv_end, "-n", "--n-couplings")) m_n_couplings = std::stoll(arg_reader);
        if (arg_reader = find_arg_name(argv, argv_end, "-1", "--couplings-one-based")) m_one_based = true;
        if (arg_reader = find_arg_name(argv, argv_end, "-r", "--run-sggs-only")) m_run_sggs_only = true;
        if (arg_reader = find_arg_value(argv, argv_end, "-b", "--block-size")) m_block_size = std::stoll(arg_reader);
        if (arg_reader = find_arg_value(argv, argv_end, "-d", "--max-distance")) m_max_distance = std::stoll(arg_reader);
        if (arg_reader = find_arg_value(argv, argv_end, "-t", "--threads")) m_n_threads = std::stoll(arg_reader);
        if (arg_reader = find_arg_name(argv, argv_end, "-v", "--verbose")) m_verbose = true;
        if (arg_reader = find_arg_value(argv, argv_end, "-F", "--filter-file")) m_filter_filename = std::string(arg_reader);
        if (arg_reader = find_arg_value(argv, argv_end, "-c", "--filter-criterion")) m_filter_criterion = std::stod(arg_reader);
        if (arg_reader = find_arg_value(argv, argv_end, "-g", "--concurrent-graphs")) m_concurrent_graphs = std::stod(arg_reader); else m_concurrent_graphs = n_threads();
        if (arg_reader = find_arg_name(argv, argv_end, "-p", "--print-unitigs")) m_print_unitigs = true;
        m_valid_state = all_required_arguments_provided();
        if (verbose()) print_arguments();
    }

    const std::string& nodes_filename() const { return m_nodes_filename; }
    const std::string& edges_filename() const { return m_edges_filename; }
    const std::string& couplings_filename() const { return m_couplings_filename; }
    const std::string& paths_filename() const { return m_paths_filename; }
    const std::string& out_stem() const { return m_out_stem; }
    const std::string& filter_filename() const { return m_filter_filename; }
    int_t k() const { return m_k; }
    int_t n_couplings() const { return m_n_couplings; }
    int_t block_size() const { return m_block_size; }
    real_t max_distance() const { return m_max_distance; }
    int_t n_threads() const { return m_n_threads; }
    int_t filter_criterion() const { return m_filter_criterion; }
    int_t concurrent_graphs() const { return m_concurrent_graphs; }
    bool one_based() const { return m_one_based; }
    bool run_sggs_only() const { return m_run_sggs_only; }
    bool verbose() const { return m_verbose; }
    bool print_unitigs() const { return m_print_unitigs; }
    bool valid_state() const { return m_valid_state; }

    const std::string out_cg_filename() const { return out_stem() + ".ud_cg_couplings"; }
    const std::string out_fcg_filename() const { return out_stem() + ".ud_fcg_couplings"; }
    const std::string out_sgg_min_filename() const { return out_stem() + ".ud_sgg_min_couplings"; }
    const std::string out_sgg_max_filename() const { return out_stem() + ".ud_sgg_max_couplings"; }
    const std::string out_sgg_mean_filename() const { return out_stem() + ".ud_sgg_mean_couplings"; }
    const std::string out_sgg_counts_filename() const { return out_stem() + ".ud_sgg_coupling_counts"; }

    // For updating the value after couplings were read.
    void set_n_couplings(int_t n_couplings) { m_n_couplings = n_couplings; }

private:
    std::string m_nodes_filename = "";
    std::string m_edges_filename = "";
    std::string m_couplings_filename = "";
    std::string m_paths_filename = "";
    std::string m_out_stem = "ud_output";
    std::string m_filter_filename = "";
    int_t m_k = 0;
    int_t m_n_couplings = INT_T_MAX;
    int_t m_block_size = 10000;
    real_t m_max_distance = REAL_T_MAX;
    int_t m_n_threads = 1;
    int_t m_filter_criterion = 2;
    int_t m_concurrent_graphs = 1;
    bool m_one_based = false;
    bool m_run_sggs_only = false;
    bool m_verbose = false;
    bool m_print_unitigs = false;
    bool m_valid_state;

    char* find_arg_name(char** begin, char** end, const std::string& opt) { return find_arg_name(begin, end, opt, opt); }
    char* find_arg_value(char** begin, char** end, const std::string& opt) { return find_arg_value(begin, end, opt, opt); }
    char* find_arg_name(char** begin, char** end, const std::string& opt, const std::string& alt) {
        // Don't need the value, return either nullptr or anything for a boolean check.
        return std::find(begin, end, opt) != end || std::find(begin, end, alt) != end ? begin[0] : nullptr;
    }
    char* find_arg_value(char** begin, char** end, const std::string& opt, const std::string& alt) {
        auto it = std::find(begin, end, opt);
        if (it != end && ++it != end) return *it;
        it = std::find(begin, end, alt);
        return it != end && ++it != end ? *it : nullptr;
    }

    bool all_required_arguments_provided() {
        bool ok = true;
        if (nodes_filename() == "") std::cout << "Missing vertices filename.\n", ok = false;
        if (edges_filename() == "") std::cout << "Missing edges filename.\n", ok = false;
        if (k() <= 0) std::cout << "Missing k-mer length.\n", ok = false;
        if (n_couplings() > 0 && !m_couplings_filename.size()) std::cout << "Missing couplings filename.\n", ok = false;
        if (!ok) print_no_args();
        return ok;
    }

    void print_no_args() { std::cout << "Use '-h' or '--help' for a list of available options.\n"; }

    void print_help() {
        std::vector<std::string> options{
            "Required arguments.", "",
            "  -V [ --vertices-file ] arg", "Path to file containing vertices.",
            "  -E [ --edges-file ] arg", "Path to file containing edges.",
            "  -k [ --k-mer-length ] arg", "k-mer length.",
            "", "",
            "Required argument if -n [ --n-couplings ] is greater than 0.", "",
            "  -C [ --couplings-file ] arg", "Path to file containing couplings.",
            "", "",
            "Optional arguments.", "",
            "  -P [ --paths-file ] arg", "Path to text file containing paths to single reference edge files.",
            "  -o [ --output-stem ] arg (=ud_output)", "Path for output files (without extension).",
            "  -n [ --n-couplings ] arg (=inf)", "Number of couplings to read from the couplings file.",
            "  -1 [ --couplings-one-based ]", "Reads couplings with one-based numbering.",
            "  -r [ --run-sggs-only ]", "Don't calculate couplings for the combined graph.",
            "  -b [ --block-size ] arg (=10000)", "Process this many couplings/tasks at a time.",
            "  -d [ --max-distance ] arg (=inf)", "Maximum allowed graph distance for constraining searches.",
            "  -t [ --threads ] arg (=1)", "Number of threads.",
            "  -v [--verbose]", "Be verbose.",
            "  -F [--filter-file] arg", "Path to file containing unitigs that will be filtered.",
            "  -c [--filter-criterion] arg (=2)", "Criterion for the filter.",
            "  -g [--concurrent-graphs] arg (=--threads)", "Number of single genome graphs to hold in memory.",
            "  -p [--print-unitigs]", "Print unitigs.",
            "  -h [ --help ]", "Print this list.",
        };
        for (auto i = 0; i < options.size(); i += 2) std::printf("%-45s %s\n", options[i].data(), options[i + 1].data());
    }

    void push_back(std::vector<std::string>& arguments, const std::string& opt, const std::string& val) {
        arguments.push_back(opt);
        arguments.push_back(val);
    }

    void print_arguments() {
        std::cout << "Printing out provided arguments.\n";
        std::vector<std::string> arguments{"  --vertices-file", nodes_filename(),"  --edges-file", edges_filename()};
        if (n_couplings() > 0) push_back(arguments, "  --couplings-file", couplings_filename());
        if (paths_filename() != "") push_back(arguments, "  --paths-file", paths_filename());
        push_back(arguments, "  --output-stem", out_stem());
        push_back(arguments, "  --k-mer-length", std::to_string(k()));
        push_back(arguments, "  --n-couplings", n_couplings() == INT_T_MAX ? "ALL" : std::to_string(n_couplings()));
        push_back(arguments, "  --run-sggs-only", run_sggs_only() ? "TRUE" : "FALSE");
        if (n_couplings() > 0) push_back(arguments, "  --couplings-one-based", one_based() ? "TRUE" : "FALSE");
        push_back(arguments, "  --block-size", std::to_string(block_size()));
        push_back(arguments, "  --max-distance", max_distance() == REAL_T_MAX ? "INF" : std::to_string(max_distance()));
        push_back(arguments, "  --threads", std::to_string(n_threads()));
        push_back(arguments, "  --verbose", verbose() ? "TRUE" : "FALSE");
        if (filter_filename() != "") push_back(arguments, "  --filter-file", filter_filename());
        if (filter_filename() != "") push_back(arguments, "  --filter-criterion", std::to_string(filter_criterion()));
        push_back(arguments, "  --concurrent-graphs", std::to_string(concurrent_graphs()));
        push_back(arguments, "  --print-unitigs", print_unitigs() ? "TRUE" : "FALSE");
        for (auto i = 0; i < arguments.size(); i += 2) std::printf("%-30s %s\n", arguments[i].data(), arguments[i + 1].data());
        std::cout << '\n';
    }

};


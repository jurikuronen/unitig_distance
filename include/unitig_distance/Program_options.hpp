/*
    Simple command line argument handler class.
*/

#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "types.hpp"

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
        if (argc < 1 + 6) { // Check if user provided enough arguments to read required arguments.
            print_no_args();
            m_valid_state = false;
            return;
        }
        // Parse required arguments.
        arg_reader = find_arg_value(argv, argv_end, "-v", "--vertices-file");
        if (arg_reader) m_nodes_filename = std::string(arg_reader);
        arg_reader = find_arg_value(argv, argv_end, "-e", "--edges-file");
        if (arg_reader) m_edges_filename = std::string(arg_reader);
        arg_reader = find_arg_value(argv, argv_end, "-c", "--couplings-file");
        if (arg_reader) m_couplings_filename = std::string(arg_reader);
        // Parse optional arguments.
        arg_reader = find_arg_value(argv, argv_end, "-o", "--output-file");
        if (arg_reader) m_out_filename = std::string(arg_reader);
        arg_reader = find_arg_value(argv, argv_end, "-n", "--n-couplings");
        if (arg_reader) m_n_couplings = std::stoll(arg_reader);
        arg_reader = find_arg_name(argv, argv_end, "-1", "--couplings-one-based");
        if (arg_reader) m_one_based = true;
        arg_reader = find_arg_name(argv, argv_end, "-s", "--smart-search");
        if (arg_reader) m_use_smart_search = true;
        arg_reader = find_arg_value(argv, argv_end, "-b", "--block-size");
        if (arg_reader) m_block_size = std::stoll(arg_reader);
        arg_reader = find_arg_value(argv, argv_end, "-d", "--max-distance");
        if (arg_reader) m_max_distance = std::stoll(arg_reader);
        arg_reader = find_arg_value(argv, argv_end, "-t", "--threads");
        if (arg_reader) m_n_threads = std::stoll(arg_reader);
        arg_reader = find_arg_name(argv, argv_end, "--verbose");
        if (arg_reader) m_verbose = true;
        m_valid_state = all_required_arguments_provided();
        if (verbose()) print_arguments();
    }

    const std::string& nodes_filename() const { return m_nodes_filename; }
    const std::string& edges_filename() const { return m_edges_filename; }
    const std::string& couplings_filename() const { return m_couplings_filename; }
    const std::string& out_filename() const { return m_out_filename; }
    int_t n_couplings() const { return m_n_couplings; }
    int_t block_size() const { return m_block_size; }
    int_t max_distance() const { return m_max_distance; }
    int_t n_threads() const { return m_n_threads; }
    bool one_based() const { return m_one_based; }
    bool use_smart_search() const { return m_use_smart_search; }
    bool verbose() const { return m_verbose; }
    bool valid_state() const { return m_valid_state; }

    // Update after couplings were read.
    void set_n_couplings(int_t n_couplings) { m_n_couplings = n_couplings; }

private:
    std::string m_nodes_filename = "";
    std::string m_edges_filename = "";
    std::string m_couplings_filename = "";
    std::string m_out_filename = "ud_output.txt";
    int_t m_n_couplings = INT_T_MAX;
    int_t m_block_size = 100000;
    int_t m_max_distance = INT_T_MAX;
    int_t m_n_threads = 1;
    bool m_use_smart_search = false;
    bool m_one_based = false;
    bool m_verbose = false;
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
        if (!m_nodes_filename.size()) std::cout << "Missing vertices filename.\n", ok = false;
        if (!m_edges_filename.size()) std::cout << "Missing edges filename.\n", ok = false;
        if (!m_couplings_filename.size()) std::cout << "Missing couplings filename.\n", ok = false;
        if (!ok) print_no_args();
        return ok;
    }

    void print_no_args() { std::cout << "Use '-h' or '--help' for a list of available options.\n"; }

    void print_help() {
        std::vector<std::string> options{
            "Required arguments.", "",
            "  -v [ --vertices-file ] arg", "Path to file containing vertices.",
            "  -e [ --edges-file ] arg", "Path to file containing edges.",
            "  -c [ --couplings-file ] arg", "Path to file containing couplings.",
            "Optional arguments.", "",
            "  -o [ --output-file ] arg (=ud_output.txt)", "Path to output file.",
            "  -n [ --n-couplings ] arg (=all)", "Number of couplings to read from the couplings file.",
            "  -1 [ --couplings-one-based ]", "Reads couplings with one-based numbering.",
            "  -s [ --smart-search ]", "Aggregate couplings into bfs tasks for smarter searches.",
            "  -b [ --block-size ] arg (=100000)", "Process this many couplings/tasks at a time (for printing).",
            "  -d [ --max-distance ] arg (=inf)", "Maximum allowed graph distance for constraining searches.",
            "  -t [ --threads ] arg (=1)", "Number of threads.",
            "  --verbose", "Be verbose.",
            "  -h [ --help ]", "Print this list."
        };
        for (auto i = 0; i < options.size(); i += 2) std::printf("%-45s %s\n", options[i].data(), options[i + 1].data());
    }

    void print_arguments() {
        std::cout << "Printing out provided arguments.\n";
        std::vector<std::string> arguments{
            "  --vertices-file", nodes_filename(),
            "  --edges-file", edges_filename(),
            "  --couplings-file", couplings_filename(),
            "  --output-file", out_filename(),
            "  --n-couplings", n_couplings() == INT_T_MAX ? "ALL" : std::to_string(n_couplings()),
            "  --couplings-one-based", one_based() ? "TRUE" : "FALSE",
            "  --smart-search", use_smart_search() ? "TRUE" : "FALSE",
            "  --block-size", std::to_string(block_size()),
            "  --max-distance", max_distance() == INT_T_MAX ? "INF" : std::to_string(max_distance()),
            "  --threads", std::to_string(n_threads()),
            "  --verbose", verbose() ? "TRUE" : "FALSE"
        };
        for (auto i = 0; i < arguments.size(); i += 2) std::printf("%-25s %s\n", arguments[i].data(), arguments[i + 1].data());
        std::cout << '\n';
    }

};


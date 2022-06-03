#pragma once

#include <string>
#include <vector>

#include "ProgramOptions.hpp"
#include "types.hpp"

class Utils {
public:
    static std::vector<std::string> get_fields(const std::string& line, char delim = ' ') {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        for (std::string field; std::getline(ss, field, delim); ) fields.push_back(std::move(field));
        return fields;
    }

    static bool file_is_good(const std::string& filename) {
        return std::ifstream(filename).good();
    }

    static std::string neat_number_str(int_t number) {
        std::vector<int_t> parts;
        do parts.push_back(number % 1000);
        while (number /= 1000);
        std::string number_str = std::to_string(parts.back());
        for (int_t i = parts.size() - 2; i >= 0; --i) {
            number_str += ' ' + std::string(3 - std::to_string(parts[i]).size(), '0') + std::to_string(parts[i]);
        }
        return number_str;
    }

    static std::string neat_decimal_str(int_t nom, int_t denom) {
        std::string int_str = std::to_string(nom / denom);
        std::string dec_str = std::to_string(nom * 100 / denom % 100);
        return int_str + "." + std::string(2 - dec_str.size(), '0') + dec_str;
    }

    static real_t fixed_distance(real_t distance, real_t max_distance = REAL_T_MAX) { return distance >= max_distance ? -1.0 : distance; }

    static bool is_numeric(const std::string& str) {
        double x;
        return (std::stringstream(str) >> x).eof();
    }

    template <typename T>
    static void clear(T& container) { T().swap(container); }

    static bool sanity_check_input_files(const ProgramOptions& po) {
        if (po.operating_mode() != OperatingMode::OUTLIER_TOOLS) {
            if (!Utils::file_is_good(po.edges_filename())) {
                std::cerr << "Can't open " << po.edges_filename() << std::endl;
                return false;
            }

            if (po.operating_mode(OperatingMode::CDBG)) {
                if (!Utils::file_is_good(po.unitigs_filename())) {
                    std::cerr << "Can't open " << po.unitigs_filename() << std::endl;
                    return false;
                }

                if (po.operating_mode(OperatingMode::SGGS)) {
                    if (!Utils::file_is_good(po.sggs_filename())) {
                        std::cerr << "Can't open " << po.sggs_filename() << std::endl;
                        return false;
                    }
                    std::ifstream ifs(po.sggs_filename());
                    for (std::string path_edges; std::getline(ifs, path_edges); ) {
                        if (!Utils::file_is_good(path_edges)) {
                            std::cerr << "Can't open " << path_edges << std::endl;
                            return false;
                        }
                    }
                }
            }
        }

        if (!po.queries_filename().empty() && po.n_queries() > 0) {
            if (!Utils::file_is_good(po.queries_filename())) {
                std::cerr << "Can't open " << po.queries_filename() << std::endl;
                return false;
            }
        }

        if (po.operating_mode() == OperatingMode::OUTLIER_TOOLS && !po.sgg_counts_filename().empty()) {
            if (!Utils::file_is_good(po.sgg_counts_filename())) {
                std::cerr << "Can't open " << po.sgg_counts_filename() << std::endl;
                return false;
            }
        }

        return true;
    }

private:

};

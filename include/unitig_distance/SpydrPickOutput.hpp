#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "types.hpp"

class SpydrPickOutput {
public:
    // Read SpydrPick output: v, w, distance, aracne_flag, mi.
    SpydrPickOutput(const std::string& spydrpick_output_filename, int_t n, bool one_based) {
        int_t v, w, aracne_flag, cnt = 0;
        real_t mi;
        std::ifstream ifs(spydrpick_output_filename);
        for (std::string line; std::getline(ifs, line); ) {
            std::stringstream ss(line);
            ss >> v >> w >> aracne_flag >> aracne_flag >> mi;
            m_queries.emplace_back(v - one_based, w - one_based);
            m_aracne_flag.push_back(aracne_flag);
            m_mi.push_back(mi);
            if (++cnt == n) break;
        }
    }

    // Output queries back with the calculated distances.
    void output_distances(const std::string& out_filename, const std::vector<real_t>& distances) {
        std::ofstream ofs(out_filename);
        ofs << "id_1 id_2 distance aracne_flag mi\n";
        for (std::size_t i = 0; i < size(); ++i) {
            int_t distance = distances[i] == REAL_T_MAX ? -1 : distances[i];
            ofs << v(i) << ' ' << w(i) << ' ' << distance << ' ' << aracne_flag(i) << ' ' << mi(i) << '\n';
        }
    }

    // Output successful query counts in the single genome graphs.
    void output_counts(const std::string& out_filename, const std::vector<int_t>& counts) {
        std::ofstream ofs(out_filename);
        ofs << "id_1 id_2 count aracne_flag mi\n";
        for (std::size_t i = 0; i < size(); ++i) {
            ofs << v(i) << ' ' << w(i) << ' ' << counts[i] << ' ' << aracne_flag(i) << ' ' << mi(i) << '\n';
        }
    }

    std::size_t size() const { return m_queries.size(); }

    int_t v(std::size_t idx) const { return m_queries[idx].first; }
    int_t w(std::size_t idx) const { return m_queries[idx].second; }
    int_t aracne_flag(std::size_t idx) const { return m_aracne_flag[idx]; }
    real_t mi(std::size_t idx) const { return m_mi[idx]; }

    typename std::vector<std::pair<int_t, int_t>>::iterator begin() { return m_queries.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::iterator end() { return m_queries.end(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator begin() const { return m_queries.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator end() const { return m_queries.end(); }

private:
    std::vector<std::pair<int_t, int_t>> m_queries;
    std::vector<real_t> m_mi;
    std::vector<int_t> m_aracne_flag;
};


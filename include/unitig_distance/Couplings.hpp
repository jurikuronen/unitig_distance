#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "types.hpp"

class Couplings {
public:
    // Read SpydrPick output: v, w, distance, aracne_flag, mi.
    Couplings(const std::string& couplings_filename, int_t n_couplings, bool one_based) {
        int_t v, w, aracne_flag, c = 0;
        real_t mi;
        std::ifstream ifs(couplings_filename);
        for (std::string line; std::getline(ifs, line); ) {
            std::stringstream ss(line);
            ss >> v >> w >> aracne_flag >> aracne_flag >> mi;
            m_couplings.emplace_back(v - one_based, w - one_based);
            m_aracne_flag.push_back(aracne_flag);
            m_mi.push_back(mi);
            if (++c == n_couplings) break;
        }
    }

    // Output couplings with the calculated distances.
    void output_distances(const std::string& out_filename, const std::vector<real_t>& distances) {
        std::ofstream ofs(out_filename);
        ofs << "id_1 id_2 distance aracne_flag mi";
        if (m_unitigs.size()) ofs << " unitig_1 unitig_2";
        ofs << "\n";
        for (auto i = 0; i < size(); ++i) {
            int_t distance = distances[i] == REAL_T_MAX ? -1 : distances[i];
            ofs << v(i) << ' ' << w(i) << ' ' << distance << ' ' << aracne_flag(i) << ' ' << mi(i);
            if (m_unitigs.size()) ofs << ' ' << unitig(v(i)) << ' ' << unitig(w(i));
            ofs << "\n";
        }
    }

    // Output connected coupling counts in the single genome graphs.
    void output_counts(const std::string& out_filename, const std::vector<int_t>& counts) {
        std::ofstream ofs(out_filename);
        ofs << "id_1 id_2 count aracne_flag mi";
        if (m_unitigs.size()) ofs << " unitig_1 unitig_2";
        ofs << "\n";
        for (auto i = 0; i < size(); ++i) {
            ofs << v(i) << ' ' << w(i) << ' ' << counts[i] << ' ' << aracne_flag(i) << ' ' << mi(i);
            if (m_unitigs.size()) ofs << ' ' << unitig(v(i)) << ' ' << unitig(w(i));
            ofs << "\n";
        }
    }

    void read_unitigs(const std::string& nodes_filename) {
        std::ifstream nodes_file(nodes_filename);
        for (std::string sequence; nodes_file >> sequence >> sequence; ) m_unitigs.push_back(sequence);
    }

    std::size_t size() const { return m_couplings.size(); }
    int_t v(std::size_t idx) const { return m_couplings[idx].first; }
    int_t w(std::size_t idx) const { return m_couplings[idx].second; }
    int_t aracne_flag(std::size_t idx) const { return m_aracne_flag[idx]; }
    real_t mi(std::size_t idx) const { return m_mi[idx]; }
    const std::string& unitig(std::size_t idx) const { return m_unitigs[idx]; }

    typename std::vector<std::pair<int_t, int_t>>::iterator begin() { return m_couplings.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::iterator end() { return m_couplings.end(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator begin() const { return m_couplings.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator end() const { return m_couplings.end(); }

private:
    std::vector<std::pair<int_t, int_t>> m_couplings;
    std::vector<real_t> m_mi;
    std::vector<int_t> m_aracne_flag;
    std::vector<std::string> m_unitigs;
};


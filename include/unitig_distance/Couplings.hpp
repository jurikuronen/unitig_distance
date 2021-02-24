/*
    Simple data structure for couplings.
*/

#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "types.hpp"

class Couplings {
public:
    Couplings(const std::string& couplings_filename, int_t n_couplings, bool one_based) {
        int_t v, w, c = 0;
        std::ifstream ifs(couplings_filename);
        std::string line;
        while (std::getline(ifs, line)) {
            std::stringstream ss(line);
            ss >> v >> w;
            m_couplings.emplace_back(v - one_based, w - one_based);
            m_lines.push_back(std::move(line));
            if (++c == n_couplings) break;
        }
    }
    std::size_t size() const { return m_couplings.size(); }
    int_t v(std::size_t idx) const { return m_couplings[idx].first; }
    int_t w(std::size_t idx) const { return m_couplings[idx].second; }
    const std::string& line(std::size_t idx) const { return m_lines[idx]; }
    typename std::vector<std::pair<int_t, int_t>>::iterator begin() { return m_couplings.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::iterator end() { return m_couplings.end(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator begin() const { return m_couplings.begin(); }
    typename std::vector<std::pair<int_t, int_t>>::const_iterator end() const { return m_couplings.end(); }

private:
    std::vector<std::pair<int_t, int_t>> m_couplings;
    std::vector<std::string> m_lines;
};

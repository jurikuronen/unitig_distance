#pragma once

#include <vector>

#include "Couplings.hpp"
#include "types.hpp"

// Distance queries for node v.
class search_job {
public:
    search_job(int_t v) : m_v(v)  { }
    const int_t v() const { return m_v; }
    const std::vector<int_t>& ws() const { return m_ws; }
    const int_t coupling_index(std::size_t idx) const { return m_coupling_indices[idx]; }
    void add(int_t w, int_t idx) {
        m_ws.push_back(w);
        m_coupling_indices.push_back(idx);
    }
    std::size_t size() const { return m_ws.size(); }

private:
    int_t m_v;
    std::vector<int_t> m_ws;
    std::vector<int_t> m_coupling_indices;

};

std::vector<search_job> compute_search_jobs(const Couplings& couplings);

/*
    Stack data structure used by the 3-edge-connectivity algorithm by Yung H. Tsin (2009).
    Stores [(x, y), p->q] tuples, where (x, y) is a (potential) generator of cut pairs with edges on the p->q path.
*/

#pragma once

#include <tuple>
#include <vector>

#include "types.hpp"

class tecc_stack {
public:
    std::tuple<int_t, int_t, int_t, int_t>& top() { return m_stack.back(); }
    int_t& top_x() { return std::get<0>(top()); }
    int_t& top_y() { return std::get<1>(top()); }
    int_t& top_p() { return std::get<2>(top()); }
    int_t& top_q() { return std::get<3>(top()); }
    std::tuple<int_t, int_t, int_t, int_t> pop() { auto t = top(); m_stack.pop_back(); return t; }
    void push(int_t x, int_t y, int_t p, int_t q) { m_stack.emplace_back(x, y, p, q); }
    void clear() { m_stack.clear(); }
    bool empty() { return m_stack.empty(); }

private:
    std::vector<std::tuple<int_t, int_t, int_t, int_t>> m_stack;
};

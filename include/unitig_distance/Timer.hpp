/*
    A time-keeping class.
*/

#pragma once

#include <chrono>
#include <ratio>
#include <string>
#include <sstream>

#include "types.hpp"

class Timer {
public:
    using clock = std::chrono::high_resolution_clock;
    Timer() : m_start(now()), m_mark(m_start) { }
    void set_mark() { m_mark = now(); }
    std::string get_time_since_start() const { return get_time_str(time_elapsed(m_start)); }
    std::string get_time_since_mark() const { return get_time_str(time_elapsed(m_mark)); }
    std::string get_time_since_start_and_set_mark() { auto time_str = get_time_since_start(); set_mark(); return time_str; }
    std::string get_time_since_mark_and_set_mark() { auto time_str = get_time_since_mark(); set_mark(); return time_str; }

private:
    clock::time_point m_start; // Set when Timer is created.
    clock::time_point m_mark; // Custom time point mark.
    clock::time_point now() const { return clock::now(); }
    clock::duration time_elapsed(const clock::time_point& tp) const { return now() - tp; }
    int_t convert_to_ms(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::milliseconds>(t).count() % 1000; }
    int_t convert_to_s(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::seconds>(t).count() % 60; }
    int_t convert_to_m(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::minutes>(t).count() % 60; }
    int_t convert_to_h(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::hours>(t).count() % 24; }
    int_t convert_to_d(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::hours>(t).count() / 24; }
    std::string get_time_str(const clock::duration& t) const {
        std::ostringstream oss;
        auto ms = convert_to_ms(t);
        auto s = convert_to_s(t);
        auto m = convert_to_m(t);
        auto h = convert_to_h(t);
        auto d = convert_to_d(t);
        if (d) oss << d << "d " << h << "h";
        else if (h) oss << h << "h " << m << "m";
        else if (m) oss << m << "m " << s << '.' << zeropadms(ms) << ms << "s";
        else oss << s << '.' << zeropadms(ms) << ms << "s";
        return oss.str();
    }
    std::string zeropadms(int_t ms) const { return ms < 10 ? "00" : (ms < 100 ? "0" : ""); }
};


#pragma once

#include <chrono>
#include <ratio>
#include <string>
#include <sstream>

#include "types.hpp"

// A time-keeping class.
class Timer {
public:
    using clock = std::chrono::high_resolution_clock;
    Timer() : m_start(now()), m_mark(m_start), m_stopwatch(clock::duration()) { }
    void set_mark() { m_mark = now(); }
    void add_time_since_mark() { m_stopwatch += time_elapsed(m_mark); }
    std::string get_stopwatch_time() const { return get_time_str(m_stopwatch); }
    std::string get_time_since_start() const { return get_time_str(time_elapsed(m_start)); }
    std::string get_time_since_mark() const { return get_time_str(time_elapsed(m_mark)); }
    std::string get_time_since_start_and_set_mark() { set_mark(); return get_time_since_start(); }
    std::string get_time_since_mark_and_set_mark() { auto time_str = get_time_since_mark(); set_mark(); return time_str; }
    std::string get_time_block_since_start() const { return get_time_block(time_elapsed(m_start)); }
    std::string get_time_block_since_mark() const { return get_time_block(time_elapsed(m_mark)); }
    std::string get_time_block_since_start_and_set_mark() { set_mark(); return get_time_block_since_start(); }
    std::string get_time_block_since_mark_and_set_mark() { auto time_block = get_time_block_since_mark(); set_mark(); return time_block; }

private:
    clock::time_point m_start; // Set when Timer is created.
    clock::time_point m_mark; // Custom time point mark.
    clock::duration m_stopwatch; // Accumulates time.
    clock::time_point now() const { return clock::now(); }
    clock::duration time_elapsed(const clock::time_point& tp) const { return now() - tp; }
    int_t convert_to_ms(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::milliseconds>(t).count() % 1000; }
    int_t convert_to_s(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::seconds>(t).count() % 60; }
    int_t convert_to_m(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::minutes>(t).count() % 60; }
    int_t convert_to_h(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::hours>(t).count() % 24; }
    int_t convert_to_d(const clock::duration& t) const { return std::chrono::duration_cast<std::chrono::hours>(t).count() / 24; }
    std::string get_time_block(const clock::duration& t) const {
        std::ostringstream oss;
        auto ms = convert_to_ms(t);
        auto s = convert_to_s(t);
        auto m = convert_to_m(t);
        auto h = convert_to_h(t);
        auto d = convert_to_d(t);
        oss << "[";
        if (d) oss << pad(d) << "d " << pad(h) << "h";
        else if (h) oss << pad(h) << "h " << pad(m) << "m";
        else if (m) oss << pad(m) << "m " << pad(s) << "s";
        else oss << pad(s) << "." << zeropad(ms) << "s";
        oss << "]";
        return oss.str();
    }
    std::string get_time_str(const clock::duration& t) const {
        auto str = get_time_block(t);
        return std::string(str.begin() + 1 + (str[1] == ' '), str.end() - 1);
    }
    std::string pad(int_t t) const { return t < 10 ? " " + std::to_string(t) : std::to_string(t); }
    std::string zeropad(int_t t) const { return t < 10 ? "00" + std::to_string(t) : (t < 100 ? "0" + std::to_string(t) : std::to_string(t)); }
};


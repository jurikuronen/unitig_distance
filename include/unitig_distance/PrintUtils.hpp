#pragma once

#include <fstream>

#include "Timer.hpp"

class PrintUtils {
public:
    static void print_license() { std::cout << "unitig_distance | MIT License | Copyright (c) 2020-2022 Juri Kuronen\n\n"; }

    template<typename... Ts>
    static void print_tbss(const Timer& timer, Ts... ts) {
        tbss(timer);
        print(ts...);
        dot_endl();
    }

    template<typename... Ts>
    static void print_tbssasm(Timer& timer, Ts... ts) {
        tbssasm(timer);
        print(ts...);
        dot_endl();
    }

    template <typename... Ts>
    static void print_tbss_tsmasm_noendl(Timer& timer, Ts... ts) {
        tbss(timer);
        print(ts...);
        tsmasm(timer);
        dot_noendl();
    }

    template <typename... Ts>
    static void print_tbss_tsm(const Timer& timer, Ts... ts) {
        tbss(timer);
        print(ts...);
        tsm(timer);
        dot_endl();
    }

    template <typename... Ts>
    static void print_tbss_tsmasm(Timer& timer, Ts... ts) {
        tbss(timer);
        print(ts...);
        tsmasm(timer);
        dot_endl();
    }

private:
    template <typename T>
    static void print(T t) { std::cout << ' ' << t; }

    template <typename T, typename... Ts>
    static void print(T t, Ts... ts) { print(t); print(ts...); }

    static void tbss(const Timer& timer) { std::cout << timer.get_time_block_since_start(); }
    static void tbssasm(Timer& timer) { std::cout << timer.get_time_block_since_start_and_set_mark(); }
    static void tsm(const Timer& timer) { print("in", timer.get_time_since_mark()); }
    static void tsmasm(Timer& timer) { print("in", timer.get_time_since_mark_and_set_mark()); }

    static void dot_endl() { std::cout << '.' << std::endl; }
    static void dot_noendl() { std::cout << ". "; }

};

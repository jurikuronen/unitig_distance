#pragma once

#include <fstream>

#include "Timer.hpp"

class PrintUtils {
public:

    static void print_license() { std::cout << "unitig_distance | MIT License | Copyright (c) 2020-2022 Juri Kuronen\n\n"; }

    template <typename T>
    static void print(T t) { std::cout << ' ' << t; }

    template <typename T, typename... Ts>
    static void print(T t, Ts... ts) { print(t); print(ts...); }

    template <typename... Ts>
    static void tbss(const Timer& timer, Ts... ts) { std::cout << timer.get_time_block_since_start(); print(ts...); }

    template <typename... Ts>
    static void tbssasm(Timer& timer, Ts... ts) { std::cout << timer.get_time_block_since_start_and_set_mark(); print(ts...); }

    static void tsmasm(Timer& timer) { print("in", timer.get_time_since_mark_and_set_mark()); }

    template<typename... Ts>
    static void print_tbss(const Timer& timer, Ts... ts) {
        tbss(timer, ts...);
        std::cout << '.' << std::endl;
    }

    template<typename... Ts>
    static void print_tbssasm(Timer& timer, Ts... ts) {
        tbssasm(timer, ts...);
        std::cout << '.' << std::endl;
    }

    template <typename... Ts>
    static void print_tbss_tsmasm_noendl(Timer& timer, Ts... ts) {
        tbss(timer, ts...);
        tsmasm(timer);
        std::cout << ". ";
    }

    template <typename... Ts>
    static void print_tbss_tsmasm(Timer& timer, Ts... ts) {
        tbss(timer, ts...);
        tsmasm(timer);
        std::cout << '.' << std::endl;
    }

    

};

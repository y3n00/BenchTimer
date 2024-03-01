#include <iostream>
#include <string>
#include <string_view>

#include "../myLibs/Random.hpp"
#include "Timer.hpp"
#ifdef IS_LINUX
#include <fmt/format.h>
using fmt::format;
#else
#include <format>
using std::format
#endif

int main() {
    static Random r;
    constexpr size_t length_base = 20;

    Bench_Timer<Measurements::Ms> timer;
    timer.add("main").start();

    for (size_t i = 1; i <= 5; i++) {
        const auto len = length_base * i;
        const auto title = format("loop #{}, size {}", i, len);
        auto& t = timer.add(title);
        t.start();
        for (size_t i = 0; i < 100'000; i++) {
            r.generate_string(len);
            t.timestamp();
        }
        t.stop();
    }
    for (auto&& [title, timer] : timer.get_all()) {
        timer.stop();
        std::cout << title << '\t';
        fmt::print("avg: {} ms\n total: {}\n\n", timer.average_time(), timer.dur());
    }
}
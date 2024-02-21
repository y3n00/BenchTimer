#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include "E:\myLibs\Random.hpp"
#include "Timer.hpp"
int main() {
    static Random r;
    constexpr size_t length_base = 20;

    bench_timer<Measurements::Ms> timer;
    timer.add("main").start();

    for (size_t i = 1; i <= 5; i++) {
        const auto len = length_base * i;
        const auto title = std::format("loop #{}, size {}", i, len);
        auto& t = timer.add(title);
        t.start();
        for (size_t i = 0; i < 100'000; i++) {
            r.generate_string(len);
            t.timestamp();
        }
        t.stop();
    }
    timer.stop_all<true>();
    auto data = timer.get_all();
    timer.remove_all();

    for (auto&& [title, timer] : data) {
        using namespace std::chrono_literals;
        std::cout << title << '\t';
        const auto timer_start = timer.start_timestamp();
        const auto& timestamps = timer.all_timestamps();
        const auto avg_diff = std::inner_product(
                                  timestamps.begin() + 1, timestamps.end(), timestamps.begin(), 0ms,
                                  [](auto&& x, auto&& y) {
                                      return x + y;
                                  },
                                  [](auto&& x, auto&& y) {
                                      return std::chrono::round<Measurements::Ms>(x - y);
                                  }) /
                              (timestamps.size() - 1);
        std::cout << avg_diff << '\n';
        std::cout << '\n';
    }
}
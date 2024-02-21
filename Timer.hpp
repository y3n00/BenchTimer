#pragma once
#include <algorithm>
#include <chrono>
#include <execution>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Measurements {
    using ns = std::chrono::nanoseconds;
    using ms = std::chrono::microseconds;
    using Ms = std::chrono::milliseconds;
    using s = std::chrono::seconds;
    using m = std::chrono::minutes;
    using h = std::chrono::hours;
}  // namespace Measurements

template <typename Ty>
concept Measurement = requires {
    requires std::chrono::_Is_duration_v<Ty>;
};

template <Measurement M>
class Timer {
   public:
    auto start_timestamp() const { return m_start; }
    auto stop_timestamp() const { return m_stop; }

    auto all_timestamps() const {
        return m_timestamps |
               std::views::transform([](auto&& dur) { return std::chrono::duration_cast<M>(dur); }) |
               std::ranges::to<std::vector<M>>();
    }

    void reset() {
        m_start = m_stop = {};
        m_timestamps.clear();
        is_running = false;
    }

    void start() {
        reset();
        m_timestamps.emplace_back(0);
        m_start = SC::now();
        is_running = true;
    }

    void stop() {
        if (!is_running)
            return;
        timestamp();
        m_stop = SC::now();
        is_running = false;
    }

    void timestamp() {
        if (is_running)
            m_timestamps.emplace_back(SC::now() - m_start);
    }

   private:
    using SC = std::chrono::steady_clock;
    using TP = std::chrono::time_point<SC>;
    TP m_start = {}, m_stop = {};
    std::vector<M> m_timestamps;
    bool is_running = false;
};

template <Measurement M>
class bench_timer {
   private:
    using timer_t = Timer<M>;
    using timers_t = std::unordered_map<std::string, timer_t>;
    timers_t m_timers;

    timer_t* find_timer_by_title(const std::string& title) {
        auto&& it = m_timers.find(title);
        auto* ptr = (it == m_timers.end() ? nullptr : &it->second);
        return ptr;
    }

    const timer_t* find_timer_by_title(const std::string& title) const {
        auto& it = m_timers.find(title);
        auto* ptr = (it == m_timers.end() ? nullptr : &it->second);
        return ptr;
    }

    template <bool parallel_flag>
    constexpr void apply_to_all(auto&& func) {
        if constexpr (parallel_flag == true)
            std::for_each(std::execution::par, m_timers.begin(), m_timers.end(), func);
        else
            std::for_each(std::execution::seq, m_timers.begin(), m_timers.end(), func);
    }

   public:
    timer_t& add(const std::string& title) {
        return (m_timers[title] = {});
    }

    void start(const std::string& title) {
        if (auto ptr = find_timer_by_title(title))
            ptr->start();
    }

    template <bool parallel_flag = true>
    constexpr void start_all() {
        apply_to_all<parallel_flag>([](auto&& iter) -> void {
            auto&& [title, timer] = iter;
            timer.start();
        });
    }

    void stop(const std::string& title) {
        if (auto ptr = find_timer_by_title(title))
            ptr->stop();
    }

    template <bool parallel_flag = true>
    constexpr void stop_all() {
        apply_to_all<parallel_flag>([](auto&& iter) -> void {
            auto&& [title, timer] = iter;
            timer.stop();
        });
    }

    void make_timestamp(const std::string& title) {
        if (auto ptr = find_timer_by_title(title))
            ptr->timestamp();
    }

    auto get_all() const { return m_timers; }

    void remove(const std::string& title) {
        if (auto it = m_timers.find(title); it != m_timers.end())
            m_timers.erase(it);
    }

    void remove_all() {
        m_timers.clear();
    }
};
#pragma once
#include <chrono>
#include <execution>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(__clang__)
#define CLANG
#define IS_LINUX
#elif defined(__GNUC__)
#define GXX
#define IS_LINUX
#elif defined(_MSC_VER)
#define MSVC
#define IS_WINDOWS
#endif

#ifdef IS_WINDOWS
#define OS_SPECIFIC(win, linux) win
#else
#define OS_SPECIFIC(win, linux) linux
#endif

#define CPP20_ENABLED OS_SPECIFIC(_HAS_CXX20, __cplusplus > 201703L)

namespace Measurements {
    using ns = std::chrono::nanoseconds;
    using ms = std::chrono::microseconds;
    using Ms = std::chrono::milliseconds;
    using s = std::chrono::seconds;
    using m = std::chrono::minutes;
    using h = std::chrono::hours;
#if CPP20_ENABLED
    using d = std::chrono::days;
    using w = std::chrono::weeks;
    using min = std::chrono::months;
    using y = std::chrono::years;
#endif
    template <typename Ty>
    concept Measurement = requires {
        requires OS_SPECIFIC(
            std::chrono::_Is_duration_v<Ty>,         // for msvc
            std::chrono::__is_duration<Ty>::value);  // for other compilers
    };
}  // namespace Measurements

namespace Exec_pol {
    using par = std::execution::parallel_policy;
    using seq = std::execution::sequenced_policy;
    using unseq = std::execution::unsequenced_policy;
    using par_unseq = std::execution::parallel_unsequenced_policy;

    template <typename Ty>
    concept Exec_policy = requires {
        requires std::is_execution_policy_v<Ty>;
    };
}  // namespace Exec_pol

template <Measurements::Measurement M>
class Timer {
    using ClockType = std::chrono::high_resolution_clock;
    using TP = std::chrono::time_point<ClockType>;

   public:
    constexpr [[nodiscard]] auto start_timestamp() const { return m_start; }
    constexpr [[nodiscard]] auto stop_timestamp() const { return m_stop; }
    constexpr [[nodiscard]] auto all_timestamps() const { return m_timestamps; }

    constexpr void reset() {
        m_start = m_stop = {};
        m_timestamps.clear();
        is_running = false;
    }

    constexpr void start() {
        reset();
        m_timestamps.emplace_back(0);
        m_start = ClockType::now();
        is_running = true;
    }

    constexpr void stop() {
        if (!is_running)
            return;
        m_stop = _timestamp();
        is_running = false;
    }

    constexpr void timestamp() {
        if (!is_running)
            return;
        _timestamp();
    }

    [[nodiscard]] constexpr double average_time() const {
        return this->dur() / static_cast<float>(m_timestamps.size());
    }

    [[nodiscard]] constexpr auto dur() const {
        return (*m_timestamps.rbegin()).count();
    }

   private:
    inline auto _timestamp() {
        const auto now = ClockType::now();
        const auto delta = std::chrono::duration_cast<M>(now - m_start).count();
        m_timestamps.emplace_back(delta);
        return now;
    }

    bool is_running = false;
    TP m_start = {}, m_stop = {};
    std::vector<M> m_timestamps{};
};

template <Measurements::Measurement M>
class Bench_Timer {
   private:
    using Timer_t = Timer<M>;
    using Timers_t = std::unordered_map<std::string, Timer_t>;
    static inline Timers_t m_timers{};

    [[nodiscard]] static inline Timer_t* _find_timer_by_title(const std::string& title) {
        auto&& it = m_timers.find(title);
        auto* timer_ptr = (it == m_timers.end() ? nullptr : &it->second);
        return timer_ptr;
    }

    template <Exec_pol::Exec_policy ExPo = Exec_pol::par>
    constexpr static void _apply_to_all(auto&& func) {
        constexpr ExPo exec_pol{};
        std::for_each(exec_pol, m_timers.begin(), m_timers.end(), func);
    }

   public:
    [[nodiscard]] static Timer_t& add(const std::string& title) {
        return (m_timers[title] = {});
    }

    static void start(const std::string& title) {
        if (auto timer_ptr = _find_timer_by_title(title))
            timer_ptr->start();
    }

    template <Exec_pol::Exec_policy ExPo = Exec_pol::par>
    constexpr static void start_all() {
        _apply_to_all<ExPo>([](auto&& iter) {
            auto&& [title, timer] = iter;
            timer.start();
        });
    }

    static void stop(const std::string& title) {
        if (auto timer_ptr = _find_timer_by_title(title))
            timer_ptr->stop();
    }

    template <Exec_pol::Exec_policy ExPo = Exec_pol::par>
    constexpr static void stop_all() {
        _apply_to_all<ExPo>([](auto&& iter) {
            auto&& [title, timer] = iter;
            timer.stop();
        });
    }

    static void make_timestamp(const std::string& title) {
        if (auto timer_ptr = _find_timer_by_title(title))
            timer_ptr->timestamp();
    }

    [[nodiscard]] static auto get_all() { return m_timers; }

    void static remove(const std::string& title) {
        if (auto it = m_timers.find(title); it != m_timers.end())
            m_timers.erase(it);
    }

    void static remove_all() { m_timers.clear(); }
};
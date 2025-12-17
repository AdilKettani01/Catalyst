#pragma once

#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// NumPy/SciPy-style utilities used throughout the VID port.
namespace vid {

using Vec2f = Eigen::Vector2f;
using Vec3f = Eigen::Vector3f;
using VecXf = Eigen::VectorXf;

struct ColorRGB {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

namespace detail {
inline constexpr float kByteToFloat = 1.0f / 255.0f;

inline constexpr uint8_t kHexInvalid = 0xFF;

inline uint8_t hexNibble(char c) noexcept {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(10 + (c - 'A'));
    return kHexInvalid;
}

inline bool parseHexByte(std::string_view s, size_t i, uint8_t& out) noexcept {
    if (i + 1 >= s.size()) return false;
    const uint8_t hi = hexNibble(s[i]);
    const uint8_t lo = hexNibble(s[i + 1]);
    if (hi == kHexInvalid || lo == kHexInvalid) return false;
    out = static_cast<uint8_t>((hi << 4) | lo);
    return true;
}

inline int toByte(float v) noexcept {
    v = std::clamp(v, 0.0f, 1.0f);
    return static_cast<int>(std::lround(v * 255.0f));
}
}  // namespace detail

inline ColorRGB rgb_from_hex(std::string_view hex) noexcept {
    if (!hex.empty() && hex.front() == '#') hex.remove_prefix(1);

    if (hex.size() == 3) {
        const uint8_t r0 = detail::hexNibble(hex[0]);
        const uint8_t g0 = detail::hexNibble(hex[1]);
        const uint8_t b0 = detail::hexNibble(hex[2]);
        if (r0 == detail::kHexInvalid || g0 == detail::kHexInvalid || b0 == detail::kHexInvalid) return {};

        const uint8_t r = static_cast<uint8_t>(r0 * 17);
        const uint8_t g = static_cast<uint8_t>(g0 * 17);
        const uint8_t b = static_cast<uint8_t>(b0 * 17);
        return {r * detail::kByteToFloat, g * detail::kByteToFloat, b * detail::kByteToFloat};
    }

    if (hex.size() < 6) return {};
    uint8_t r = 0, g = 0, b = 0;
    if (!detail::parseHexByte(hex, 0, r) || !detail::parseHexByte(hex, 2, g) || !detail::parseHexByte(hex, 4, b)) {
        return {};
    }
    return {r * detail::kByteToFloat, g * detail::kByteToFloat, b * detail::kByteToFloat};
}

inline std::string hex_from_rgb(ColorRGB color) {
    const int r = detail::toByte(color.r);
    const int g = detail::toByte(color.g);
    const int b = detail::toByte(color.b);
    char out[8] = {};
    std::snprintf(out, sizeof(out), "#%02X%02X%02X", r, g, b);
    return std::string(out);
}

template <typename Scalar>
requires(std::is_arithmetic_v<Scalar>)
constexpr Scalar interpolate(Scalar a, Scalar b, double alpha) noexcept {
    return static_cast<Scalar>(a + (b - a) * alpha);
}

template <typename Derived>
inline typename Derived::PlainObject interpolate(const Eigen::MatrixBase<Derived>& a,
                                                 const Eigen::MatrixBase<Derived>& b,
                                                 double alpha) {
    return (a + (b - a) * alpha).eval();
}

inline ColorRGB interpolate(ColorRGB a, ColorRGB b, double alpha) noexcept {
    return {
        interpolate(a.r, b.r, alpha),
        interpolate(a.g, b.g, alpha),
        interpolate(a.b, b.b, alpha),
    };
}

template <typename Scalar>
requires(std::is_arithmetic_v<Scalar>)
constexpr double inverse_interpolate(Scalar a, Scalar b, Scalar x) noexcept {
    if (a == b) return 0.0;
    return static_cast<double>(x - a) / static_cast<double>(b - a);
}

template <typename Scalar>
requires(std::is_arithmetic_v<Scalar>)
constexpr Scalar match_interpolate(Scalar a, Scalar b, Scalar c, Scalar d, Scalar x) noexcept {
    return interpolate(c, d, inverse_interpolate(a, b, x));
}

template <typename Scalar>
requires(std::is_arithmetic_v<Scalar>)
inline std::vector<Scalar> linspace(Scalar start, Scalar stop, std::size_t num, bool endpoint = true) {
    std::vector<Scalar> out;
    out.reserve(num);
    if (num == 0) return out;
    if (num == 1) {
        out.push_back(start);
        return out;
    }

    const double denom = endpoint ? static_cast<double>(num - 1) : static_cast<double>(num);
    const double startD = static_cast<double>(start);
    const double stopD = static_cast<double>(stop);
    const double step = (stopD - startD) / denom;
    for (std::size_t i = 0; i < num; ++i) {
        out.push_back(static_cast<Scalar>(startD + step * static_cast<double>(i)));
    }
    if (endpoint) out.back() = stop;
    return out;
}

template <typename Scalar>
requires(std::is_arithmetic_v<Scalar>)
inline Eigen::Matrix<Scalar, Eigen::Dynamic, 1> linspace_eigen(Scalar start,
                                                               Scalar stop,
                                                               Eigen::Index num,
                                                               bool endpoint = true) {
    Eigen::Matrix<Scalar, Eigen::Dynamic, 1> out;
    if (num <= 0) return out;
    out.resize(num);

    if (num == 1) {
        out(0) = start;
        return out;
    }

    const double denom = endpoint ? static_cast<double>(num - 1) : static_cast<double>(num);
    const double startD = static_cast<double>(start);
    const double stopD = static_cast<double>(stop);
    const double step = (stopD - startD) / denom;
    for (Eigen::Index i = 0; i < num; ++i) {
        out(i) = static_cast<Scalar>(startD + step * static_cast<double>(i));
    }
    if (endpoint) out(num - 1) = stop;
    return out;
}

template <typename Scalar>
requires(std::is_arithmetic_v<Scalar>)
inline std::vector<Scalar> arange(Scalar start, Scalar stop, Scalar step = static_cast<Scalar>(1)) {
    std::vector<Scalar> out;
    const double stepD = static_cast<double>(step);
    if (stepD == 0.0) return out;

    const double startD = static_cast<double>(start);
    const double stopD = static_cast<double>(stop);
    const double span = stopD - startD;
    const double nRaw = span / stepD;
    if (nRaw <= 0.0) return out;

    const std::size_t n = static_cast<std::size_t>(std::ceil(nRaw));
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(static_cast<Scalar>(startD + stepD * static_cast<double>(i)));
    }
    return out;
}

template <typename Scalar>
requires(std::is_arithmetic_v<Scalar>)
inline std::vector<Scalar> cumsum(const std::vector<Scalar>& values) {
    std::vector<Scalar> out;
    out.reserve(values.size());
    Scalar sum = static_cast<Scalar>(0);
    for (const Scalar& v : values) {
        sum = static_cast<Scalar>(sum + v);
        out.push_back(sum);
    }
    return out;
}

inline std::vector<ColorRGB> color_gradient(const std::vector<ColorRGB>& colors, std::size_t length) {
    std::vector<ColorRGB> out;
    out.reserve(length);
    if (length == 0 || colors.empty()) return out;
    if (colors.size() == 1) {
        out.assign(length, colors.front());
        return out;
    }
    if (length == 1) {
        out.push_back(colors.front());
        return out;
    }

    const double maxColorIndex = static_cast<double>(colors.size() - 1);
    const double denom = static_cast<double>(length - 1);
    for (std::size_t i = 0; i < length; ++i) {
        const double a = (static_cast<double>(i) / denom) * maxColorIndex;
        const std::size_t idx = static_cast<std::size_t>(std::floor(a));
        const double frac = a - static_cast<double>(idx);
        if (idx >= colors.size() - 1) {
            out.push_back(colors.back());
        } else {
            out.push_back(interpolate(colors[idx], colors[idx + 1], frac));
        }
    }
    return out;
}

inline std::vector<ColorRGB> color_gradient(std::initializer_list<ColorRGB> colors, std::size_t length) {
    return color_gradient(std::vector<ColorRGB>(colors), length);
}

inline std::vector<std::string> color_gradient_hex(std::initializer_list<std::string_view> colors, std::size_t length) {
    std::vector<ColorRGB> rgb;
    rgb.reserve(colors.size());
    for (std::string_view c : colors) rgb.push_back(rgb_from_hex(c));

    const auto grad = color_gradient(rgb, length);
    std::vector<std::string> out;
    out.reserve(grad.size());
    for (const auto& c : grad) out.push_back(hex_from_rgb(c));
    return out;
}

template <typename State, typename DerivFn>
inline State euler_step(DerivFn&& f, double t, const State& y, double dt) {
    return y + f(t, y) * dt;
}

template <typename State, typename DerivFn>
inline State rk4_step(DerivFn&& f, double t, const State& y, double dt) {
    const State k1 = f(t, y);
    const State k2 = f(t + 0.5 * dt, y + k1 * (0.5 * dt));
    const State k3 = f(t + 0.5 * dt, y + k2 * (0.5 * dt));
    const State k4 = f(t + dt, y + k3 * dt);
    return y + (k1 + k2 * 2.0 + k3 * 2.0 + k4) * (dt / 6.0);
}

template <typename State>
struct ODESolution {
    std::vector<double> t;
    std::vector<State> y;
};

template <typename State, typename DerivFn>
inline ODESolution<State> solve_ivp_rk4(DerivFn&& f,
                                       double t0,
                                       double t1,
                                       const State& y0,
                                       double dt,
                                       const std::vector<double>& t_eval = {}) {
    ODESolution<State> sol;
    if (dt <= 0.0) return sol;
    if (t1 < t0) return sol;

    if (t_eval.empty()) {
        const std::size_t steps = static_cast<std::size_t>(std::ceil((t1 - t0) / dt));
        sol.t.reserve(steps + 1);
        sol.y.reserve(steps + 1);

        double t = t0;
        State y = y0;
        sol.t.push_back(t);
        sol.y.push_back(y);
        for (std::size_t i = 0; i < steps; ++i) {
            const double step = std::min(dt, t1 - t);
            y = rk4_step<State>(f, t, y, step);
            t += step;
            sol.t.push_back(t);
            sol.y.push_back(y);
            if (t >= t1) break;
        }
        return sol;
    }

    sol.t.reserve(t_eval.size());
    sol.y.reserve(t_eval.size());

    constexpr double kEps = 1e-12;
    std::size_t evalIdx = 0;
    double t = t0;
    State y = y0;

    while (evalIdx < t_eval.size() && t_eval[evalIdx] <= t0 + kEps) {
        sol.t.push_back(t_eval[evalIdx]);
        sol.y.push_back(y0);
        ++evalIdx;
    }

    while (t < t1 - kEps && evalIdx < t_eval.size()) {
        const double step = std::min(dt, t1 - t);
        const double tNext = t + step;
        const State yNext = rk4_step<State>(f, t, y, step);

        while (evalIdx < t_eval.size() && t_eval[evalIdx] <= tNext + kEps) {
            const double alpha = (step > 0.0) ? ((t_eval[evalIdx] - t) / step) : 0.0;
            sol.t.push_back(t_eval[evalIdx]);
            sol.y.push_back(interpolate(y, yNext, alpha));
            ++evalIdx;
        }

        t = tNext;
        y = yNext;
    }

    return sol;
}

template <typename Y>
class LinearInterpolator {
public:
    LinearInterpolator() = default;

    LinearInterpolator(std::vector<double> xs, std::vector<Y> ys, bool extrapolate = false)
        : xs_(std::move(xs)), ys_(std::move(ys)), extrapolate_(extrapolate) {
        const std::size_t n = std::min(xs_.size(), ys_.size());
        xs_.resize(n);
        ys_.resize(n);
    }

    [[nodiscard]] bool empty() const noexcept { return xs_.empty() || ys_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return std::min(xs_.size(), ys_.size()); }

    [[nodiscard]] Y operator()(double x) const {
        if (xs_.size() < 2 || ys_.size() < 2) return {};

        if (x <= xs_.front()) {
            if (!extrapolate_) return ys_.front();
            return interpolate(ys_[0], ys_[1], inverse_interpolate(xs_[0], xs_[1], x));
        }
        if (x >= xs_.back()) {
            if (!extrapolate_) return ys_.back();
            const std::size_t n = xs_.size();
            return interpolate(ys_[n - 2], ys_[n - 1], inverse_interpolate(xs_[n - 2], xs_[n - 1], x));
        }

        const auto it = std::upper_bound(xs_.begin(), xs_.end(), x);
        const std::size_t i1 = static_cast<std::size_t>(std::distance(xs_.begin(), it));
        const std::size_t i0 = i1 - 1;
        return interpolate(ys_[i0], ys_[i1], inverse_interpolate(xs_[i0], xs_[i1], x));
    }

private:
    std::vector<double> xs_;
    std::vector<Y> ys_;
    bool extrapolate_ = false;
};

}  // namespace vid

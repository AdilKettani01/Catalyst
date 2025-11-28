#pragma once

#include <functional>
#include <vector>
#include <string>

namespace manim {
namespace RateFunctions {

// Rate function type
using RateFunction = std::function<float(float)>;

// Standard rate functions
float linear(float t);
float smooth(float t);
float smootherStep(float t);
float rushInto(float t);
float rushFrom(float t);
float slowInto(float t);
float doubleSmooth(float t);
float thereAndBack(float t);
float thereAndBackWithPause(float t);
float runningStart(float t);
float wiggle(float t);
float exponentialDecay(float t);

// Easing functions
float easeInSine(float t);
float easeOutSine(float t);
float easeInOutSine(float t);
float easeInQuad(float t);
float easeOutQuad(float t);
float easeInOutQuad(float t);
float easeInCubic(float t);
float easeOutCubic(float t);
float easeInOutCubic(float t);
float easeInExpo(float t);
float easeOutExpo(float t);
float easeInOutExpo(float t);
float easeInElastic(float t);
float easeOutElastic(float t);
float easeInOutElastic(float t);

// GPU batch evaluation
std::vector<float> batchEvaluate(RateFunction func, const std::vector<float>& t_values, bool useGPU = true);

inline std::vector<float> batch_evaluate(RateFunction func, const std::vector<float>& t_values, bool useGPU = true) {
    return batchEvaluate(func, t_values, useGPU);
}

// Rate function by name
RateFunction getRateFunction(const std::string& name);

// Inline stub implementations
inline float linear(float t) { return t; }
inline float smooth(float t) { return t; }
inline float smootherStep(float t) { return t; }
inline float rushInto(float t) { return t; }
inline float rushFrom(float t) { return t; }
inline float slowInto(float t) { return t; }
inline float doubleSmooth(float t) { return t; }
inline float thereAndBack(float t) { return t; }
inline float thereAndBackWithPause(float t) { return t; }
inline float runningStart(float t) { return t; }
inline float wiggle(float t) { return t; }
inline float exponentialDecay(float t) { return t; }
inline float easeInSine(float t) { return t; }
inline float easeOutSine(float t) { return t; }
inline float easeInOutSine(float t) { return t; }
inline float easeInQuad(float t) { return t; }
inline float easeOutQuad(float t) { return t; }
inline float easeInOutQuad(float t) { return t; }
inline float easeInCubic(float t) { return t; }
inline float easeOutCubic(float t) { return t; }
inline float easeInOutCubic(float t) { return t; }
inline float easeInExpo(float t) { return t; }
inline float easeOutExpo(float t) { return t; }
inline float easeInOutExpo(float t) { return t; }
inline float easeInElastic(float t) { return t; }
inline float easeOutElastic(float t) { return t; }
inline float easeInOutElastic(float t) { return t; }
inline std::vector<float> batchEvaluate(RateFunction func, const std::vector<float>& t_values, bool useGPU) {
    (void)func; (void)useGPU;
    return t_values;
}
inline RateFunction getRateFunction(const std::string&) { return linear; }

} // namespace RateFunctions
} // namespace manim

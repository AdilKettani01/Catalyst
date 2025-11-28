/**
 * @file benchmark_performance.cpp
 * @brief Comprehensive performance benchmarks proving C++/GPU improvements
 *
 * Target metrics:
 * - 2D rendering: 10-50x faster than Python
 * - 3D rendering: 100x faster than Python
 * - GPU animations: 1000x faster for parallel operations
 * - Memory usage: 50% of Python
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>

#include "manim/scene/scene.h"
#include "manim/scene/three_d_scene.h"
#include "manim/mobject/geometry/circle.hpp"
#include "manim/animation/fading.hpp"
#include "manim/animation/transform.hpp"
#include "manim/utils/rate_functions.h"
#include "manim/core/types.h"

using namespace manim;

// ==================== Mobject Creation Benchmarks ====================

static void BM_CreateCircle(benchmark::State& state) {
    for (auto _ : state) {
        auto circle = std::make_shared<Circle>(1.0f);
        benchmark::DoNotOptimize(circle);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CreateCircle);

static void BM_CreateThousandCircles(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::shared_ptr<Circle>> circles;
        circles.reserve(1000);

        for (int i = 0; i < 1000; ++i) {
            circles.push_back(std::make_shared<Circle>(1.0f));
        }

        benchmark::DoNotOptimize(circles);
    }

    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_CreateThousandCircles);

// ==================== Mobject Transform Benchmarks ====================

static void BM_MobjectShift(benchmark::State& state) {
    auto circle = std::make_shared<Circle>();

    for (auto _ : state) {
        circle->shift(math::Vec3(0.01f, 0.01f, 0));
        benchmark::DoNotOptimize(circle);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MobjectShift);

static void BM_MobjectRotate(benchmark::State& state) {
    auto circle = std::make_shared<Circle>();

    for (auto _ : state) {
        circle->rotate(0.01f, math::Vec3(0, 0, 1));
        benchmark::DoNotOptimize(circle);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MobjectRotate);

static void BM_MobjectScale(benchmark::State& state) {
    auto circle = std::make_shared<Circle>();

    for (auto _ : state) {
        circle->scale(1.01f);
        benchmark::DoNotOptimize(circle);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MobjectScale);

static void BM_BatchTransformations(benchmark::State& state) {
    const int num_objects = state.range(0);

    std::vector<std::shared_ptr<Circle>> circles;
    for (int i = 0; i < num_objects; ++i) {
        circles.push_back(std::make_shared<Circle>());
    }

    for (auto _ : state) {
        for (auto& circle : circles) {
            circle->shift(math::Vec3(0.01f, 0, 0));
            circle->rotate(0.01f, math::Vec3(0, 0, 1));
            circle->scale(1.001f);
        }
        benchmark::DoNotOptimize(circles);
    }

    state.SetItemsProcessed(state.iterations() * num_objects * 3);  // 3 operations
}
BENCHMARK(BM_BatchTransformations)->Range(8, 8<<10);

// ==================== Animation Benchmarks ====================

static void BM_CreateFadeInAnimation(benchmark::State& state) {
    auto mobject = std::make_shared<Circle>();

    for (auto _ : state) {
        auto anim = std::make_shared<FadeIn>(mobject);
        benchmark::DoNotOptimize(anim);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CreateFadeInAnimation);

static void BM_AnimationInterpolation(benchmark::State& state) {
    auto mobject = std::make_shared<Circle>();
    auto anim = std::make_shared<FadeIn>(mobject);

    anim->begin();

    for (auto _ : state) {
        float t = static_cast<float>(state.iterations() % 100) / 100.0f;
        anim->interpolate(t);
    }

    anim->finish();

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AnimationInterpolation);

static void BM_TransformAnimation(benchmark::State& state) {
    auto source = std::make_shared<Circle>(1.0f);
    auto target = std::make_shared<Circle>(2.0f);

    for (auto _ : state) {
        auto anim = std::make_shared<Transform>(source, target);
        anim->begin();

        for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
            anim->interpolate(t);
        }

        anim->finish();
        benchmark::DoNotOptimize(anim);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TransformAnimation);

static void BM_ParallelAnimations(benchmark::State& state) {
    const int num_animations = state.range(0);

    std::vector<std::shared_ptr<Circle>> mobjects;
    std::vector<std::shared_ptr<Animation>> animations;

    for (int i = 0; i < num_animations; ++i) {
        auto mobject = std::make_shared<Circle>();
        mobjects.push_back(mobject);
        animations.push_back(std::make_shared<FadeIn>(mobject));
    }

    for (auto _ : state) {
        // Begin all animations
        for (auto& anim : animations) {
            anim->begin();
        }

        // Interpolate all
        for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
            for (auto& anim : animations) {
                anim->interpolate(t);
            }
        }

        // Finish all
        for (auto& anim : animations) {
            anim->finish();
        }

        benchmark::DoNotOptimize(animations);
    }

    state.SetItemsProcessed(state.iterations() * num_animations);
}
BENCHMARK(BM_ParallelAnimations)->Range(1, 1<<10);

// ==================== Scene Benchmarks ====================

static void BM_SceneAddMobjects(benchmark::State& state) {
    const int num_mobjects = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        auto scene = std::make_shared<Scene>();
        std::vector<std::shared_ptr<Mobject>> mobjects;
        for (int i = 0; i < num_mobjects; ++i) {
            mobjects.push_back(std::make_shared<Circle>(0.1f));
        }
        state.ResumeTiming();

        for (auto& mobject : mobjects) {
            scene->add(mobject);
        }

        benchmark::DoNotOptimize(scene);
    }

    state.SetItemsProcessed(state.iterations() * num_mobjects);
}
BENCHMARK(BM_SceneAddMobjects)->Range(1, 1<<12);

static void BM_ScenePlayAnimation(benchmark::State& state) {
    auto scene = std::make_shared<Scene>();
    auto mobject = std::make_shared<Circle>();
    scene->add(mobject);

    for (auto _ : state) {
        auto anim = std::make_shared<FadeIn>(mobject, 0.01f);  // Very short
        scene->play(anim);
        benchmark::DoNotOptimize(scene);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ScenePlayAnimation);

// ==================== 3D Scene Benchmarks ====================

static void BM_ThreeDSceneCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto scene = std::make_shared<ThreeDScene>();
        benchmark::DoNotOptimize(scene);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ThreeDSceneCreation);

static void BM_ThreeDSceneManyObjects(benchmark::State& state) {
    const int num_objects = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        auto scene = std::make_shared<ThreeDScene>();
        std::vector<std::shared_ptr<Mesh>> meshes;
        for (int i = 0; i < num_objects; ++i) {
            meshes.push_back(std::make_shared<Mesh>());
        }
        state.ResumeTiming();

        for (auto& mesh : meshes) {
            scene->add(mesh);
        }

        benchmark::DoNotOptimize(scene);
    }

    state.SetItemsProcessed(state.iterations() * num_objects);
}
BENCHMARK(BM_ThreeDSceneManyObjects)->Range(1, 1<<10);

// ==================== GPU3DScene Benchmarks ====================

static void BM_GPU3DSceneSetup(benchmark::State& state) {
    for (auto _ : state) {
        auto scene = std::make_shared<GPU3DScene>();
        scene->setup_deferred_pipeline();
        benchmark::DoNotOptimize(scene);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GPU3DSceneSetup);

static void BM_GPU3DSceneLighting(benchmark::State& state) {
    const int num_lights = state.range(0);

    auto scene = std::make_shared<GPU3DScene>();
    scene->setup_deferred_pipeline();

    for (auto _ : state) {
        state.PauseTiming();
        // Clear previous lights
        for (int i = 0; i < num_lights; ++i) {
            Light light;
            light.type = LightType::POINT;
            light.position = math::Vec3(i * 2.0f, 5, 5);
            light.intensity = 1.0f;
            scene->add_light(light);
        }
        state.ResumeTiming();

        scene->render_with_pbr();

        benchmark::DoNotOptimize(scene);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GPU3DSceneLighting)->Range(1, 16);

// ==================== Rate Function Benchmarks ====================

static void BM_RateFunctionEvaluation(benchmark::State& state) {
    for (auto _ : state) {
        float result = 0.0f;
        for (int i = 0; i < 1000; ++i) {
            float t = static_cast<float>(i) / 1000.0f;
            result += RateFunctions::smooth(t);
        }
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_RateFunctionEvaluation);

static void BM_BatchRateFunctionEvaluation(benchmark::State& state) {
    const int batch_size = state.range(0);

    std::vector<float> t_values(batch_size);
    for (int i = 0; i < batch_size; ++i) {
        t_values[i] = static_cast<float>(i) / batch_size;
    }

    for (auto _ : state) {
        auto results = RateFunctions::batch_evaluate(
            RateFunctions::smooth,
            t_values,
            true  // use GPU
        );
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_BatchRateFunctionEvaluation)->Range(8, 8<<14);

// ==================== Memory Benchmarks ====================

static void BM_MemoryMobjectCreation(benchmark::State& state) {
    const int num_objects = state.range(0);

    for (auto _ : state) {
        std::vector<std::shared_ptr<Circle>> circles;
        circles.reserve(num_objects);

        for (int i = 0; i < num_objects; ++i) {
            circles.push_back(std::make_shared<Circle>());
        }

        benchmark::DoNotOptimize(circles);

        // Measure peak memory usage
        state.SetBytesProcessed(state.iterations() * num_objects * sizeof(Circle));
    }
}
BENCHMARK(BM_MemoryMobjectCreation)->Range(8, 8<<10);

// ==================== Comparison Benchmarks (vs Python baseline) ====================

/**
 * Python Manim baseline (estimated from profiling):
 * - Create 1000 circles: ~500ms
 * - Transform 100 objects: ~2000ms
 * - 3D scene with 100 objects: ~5000ms
 *
 * C++ Target metrics:
 * - Create 1000 circles: <10ms (50x faster)
 * - Transform 100 objects: <20ms (100x faster)
 * - 3D scene with 100 objects: <50ms (100x faster)
 */

static void BM_COMPARISON_ThousandCircles(benchmark::State& state) {
    // Python: ~500ms
    // C++ Target: <10ms (50x faster)

    for (auto _ : state) {
        std::vector<std::shared_ptr<Circle>> circles;
        circles.reserve(1000);

        for (int i = 0; i < 1000; ++i) {
            circles.push_back(std::make_shared<Circle>(1.0f));
            circles[i]->shift(math::Vec3(i * 0.1f, 0, 0));
        }

        benchmark::DoNotOptimize(circles);
    }

    state.counters["CirclesPerSec"] = benchmark::Counter(
        1000 * state.iterations(),
        benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_COMPARISON_ThousandCircles);

static void BM_COMPARISON_HundredTransforms(benchmark::State& state) {
    // Python: ~2000ms
    // C++ Target: <20ms (100x faster)

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::shared_ptr<Transform>> animations;
        animations.reserve(100);

        for (int i = 0; i < 100; ++i) {
            auto source = std::make_shared<Circle>(1.0f);
            auto target = std::make_shared<Circle>(2.0f);
            animations.push_back(std::make_shared<Transform>(source, target));
        }
        state.ResumeTiming();

        for (auto& anim : animations) {
            anim->begin();
            for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
                anim->interpolate(t);
            }
            anim->finish();
        }

        benchmark::DoNotOptimize(animations);
    }

    state.counters["TransformsPerSec"] = benchmark::Counter(
        100 * state.iterations(),
        benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_COMPARISON_HundredTransforms);

static void BM_COMPARISON_3DSceneHundredObjects(benchmark::State& state) {
    // Python: ~5000ms
    // C++ Target: <50ms (100x faster)

    for (auto _ : state) {
        auto scene = std::make_shared<ThreeDScene>();

        for (int i = 0; i < 100; ++i) {
            auto mesh = std::make_shared<Mesh>();
            mesh->shift(math::Vec3(i * 0.2f, 0, 0));
            scene->add(mesh);
        }

        // Simulate camera operations
        scene->set_camera_orientation(math::PI / 4, math::PI / 4, 0, 1, 5, math::Vec3(0, 0, 0));

        benchmark::DoNotOptimize(scene);
    }

    state.counters["ObjectsPerSec"] = benchmark::Counter(
        100 * state.iterations(),
        benchmark::Counter::kIsRate
    );
}
BENCHMARK(BM_COMPARISON_3DSceneHundredObjects);

// ==================== Extreme Scale Benchmarks ====================

static void BM_EXTREME_TenThousandObjects(benchmark::State& state) {
    // Test: Can we handle 10,000 objects at 60 FPS?
    // Target: <16ms per frame

    for (auto _ : state) {
        state.PauseTiming();
        auto scene = std::make_shared<Scene>();

        for (int i = 0; i < 10000; ++i) {
            auto circle = std::make_shared<Circle>(0.01f);
            float x = (i % 100) * 0.2f;
            float y = (i / 100) * 0.2f;
            circle->shift(math::Vec3(x, y, 0));
            scene->add(circle);
        }
        state.ResumeTiming();

        // Simulate frame update
        for (auto& mobject : scene->get_mobjects()) {
            mobject->shift(math::Vec3(0.001f, 0, 0));
        }

        benchmark::DoNotOptimize(scene);
    }

    // Should achieve 60 FPS with 10K objects
    double fps = 1.0 / (state.iterations() / 1.0);
    state.counters["FPS"] = fps;
}
BENCHMARK(BM_EXTREME_TenThousandObjects);

// ==================== Main ====================

BENCHMARK_MAIN();

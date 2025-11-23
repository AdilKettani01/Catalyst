# Python to C++ Pattern Conversion Guide

## Overview
This document identifies Python-specific patterns in Manim and their C++ equivalents, focusing on modern C++20/23 idioms and GPU-first design.

---

## 1. Dynamic Typing → Static Typing with Concepts

### Python Pattern
```python
def animate(obj, duration=1.0):
    # obj can be any type
    if hasattr(obj, 'interpolate'):
        obj.interpolate(0.5)
```

### C++ Solution
```cpp
// C++20 concepts for compile-time type checking
template<typename T>
concept Animatable = requires(T obj, float alpha) {
    { obj.interpolate(alpha) } -> std::same_as<void>;
    { obj.get_points() } -> std::convertible_to<std::span<glm::vec3>>;
};

template<Animatable T>
void animate(T& obj, float duration = 1.0f) {
    obj.interpolate(0.5f);
}
```

**Benefits**: Compile-time type safety, better error messages, zero runtime overhead.

---

## 2. Duck Typing → Type Traits

### Python Pattern
```python
def process_mobjects(mobs):
    for mob in mobs:
        if isinstance(mob, VMobject):
            mob.set_stroke()
        elif isinstance(mob, PMobject):
            mob.set_points()
```

### C++ Solution
```cpp
// Type-safe variant approach
using MobjectVariant = std::variant<VMobject*, PMobject*, ImageMobject*>;

void process_mobjects(std::span<MobjectVariant> mobs) {
    for (auto& mob_var : mobs) {
        std::visit([]<typename T>(T* mob) {
            if constexpr (std::is_same_v<T, VMobject>) {
                mob->set_stroke();
            } else if constexpr (std::is_same_v<T, PMobject>) {
                mob->set_points();
            }
        }, mob_var);
    }
}

// Or use polymorphism with virtual functions
void process_mobjects(std::span<Mobject*> mobs) {
    for (auto* mob : mobs) {
        mob->update();  // Virtual dispatch
    }
}
```

---

## 3. List Comprehensions → Ranges (C++20/23)

### Python Pattern
```python
points = [normalize(p) for p in self.points if np.linalg.norm(p) > 0.1]
colors = [interpolate_color(c1, c2, alpha) for alpha in np.linspace(0, 1, 100)]
```

### C++ Solution
```cpp
#include <ranges>

// Filter and transform
auto points = self.points
    | std::views::filter([](const glm::vec3& p) { return glm::length(p) > 0.1f; })
    | std::views::transform([](const glm::vec3& p) { return glm::normalize(p); })
    | std::ranges::to<std::vector>();

// Generate and transform
auto alphas = std::views::iota(0, 100)
    | std::views::transform([](int i) { return i / 99.0f; });

auto colors = alphas
    | std::views::transform([&](float alpha) {
        return interpolate_color(c1, c2, alpha);
    })
    | std::ranges::to<std::vector>();
```

---

## 4. NumPy Arrays → Eigen/GLM + GPU Buffers

### Python Pattern
```python
import numpy as np

class Mobject:
    def __init__(self):
        self.points = np.zeros((100, 3))
        self.colors = np.ones((100, 4))

    def transform(self, matrix):
        self.points = self.points @ matrix.T  # Matrix multiplication
```

### C++ Solution
```cpp
#include <Eigen/Dense>
#include <glm/glm.hpp>

class Mobject {
public:
    // CPU-side data (small datasets)
    Eigen::Matrix<float, Eigen::Dynamic, 3> points_eigen;

    // Or use std::vector with GLM for GPU compatibility
    std::vector<glm::vec3> points;
    std::vector<glm::vec4> colors;

    // GPU-side data (primary for large datasets)
    VkBuffer gpu_vertex_buffer;
    VkDeviceMemory gpu_vertex_memory;
    bool gpu_dirty = true;

    void transform(const glm::mat4& matrix) {
        // CPU path (small data)
        for (auto& p : points) {
            p = glm::vec3(matrix * glm::vec4(p, 1.0f));
        }
        gpu_dirty = true;

        // GPU path (large data) - dispatch compute shader
        if (points.size() > 10000) {
            transform_gpu(matrix);
        }
    }

private:
    void transform_gpu(const glm::mat4& matrix) {
        // Upload matrix to uniform buffer
        // Dispatch compute shader
        // No CPU-side modification needed
    }
};
```

**Key Differences**:
- **NumPy**: Everything on CPU, automatic memory management
- **C++**: Explicit CPU/GPU split, manual memory management (or smart pointers)
- **GPU-first**: Keep large data on GPU, minimize CPU-GPU transfers

---

## 5. Property Decorators → Getter/Setter or Public Members

### Python Pattern
```python
class Mobject:
    def __init__(self):
        self._color = WHITE

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, value):
        self._color = value
        self.update_shaders()
```

### C++ Solution

#### Option 1: Getters/Setters (OOP style)
```cpp
class Mobject {
private:
    Color m_color = WHITE;

public:
    const Color& get_color() const { return m_color; }

    void set_color(const Color& color) {
        m_color = color;
        update_shaders();
    }
};
```

#### Option 2: Public Members with Proxy (Modern C++)
```cpp
template<typename T, typename OnChange>
class Property {
    T value;
    OnChange on_change;

public:
    Property(T val, OnChange oc) : value(val), on_change(oc) {}

    operator const T&() const { return value; }

    Property& operator=(const T& new_val) {
        value = new_val;
        on_change(value);
        return *this;
    }
};

class Mobject {
public:
    Property<Color, std::function<void(const Color&)>> color{
        WHITE,
        [this](const Color& c) { this->update_shaders(); }
    };
};

// Usage
mob.color = RED;  // Automatically calls update_shaders()
```

---

## 6. Multiple Inheritance → Composition or Mixins via CRTP

### Python Pattern
```python
class MovingCameraScene(Scene, MovingCamera):
    def __init__(self):
        Scene.__init__(self)
        MovingCamera.__init__(self)
```

### C++ Solution

#### Option 1: Composition (Preferred)
```cpp
class MovingCameraScene : public Scene {
    MovingCamera camera;

public:
    MovingCameraScene() : Scene(), camera() {}

    void update(float dt) override {
        Scene::update(dt);
        camera.update(dt);
    }
};
```

#### Option 2: CRTP for Mixins
```cpp
// Curiously Recurring Template Pattern
template<typename Derived>
class MovingCameraMixin {
protected:
    void move_camera(const glm::vec3& target) {
        // Access derived class
        auto& self = static_cast<Derived&>(*this);
        self.camera.move_to(target);
    }
};

class MovingCameraScene : public Scene, public MovingCameraMixin<MovingCameraScene> {
public:
    Camera camera;

    void some_method() {
        move_camera(glm::vec3(1, 0, 0));
    }
};
```

---

## 7. Dynamic Function Binding → Function Pointers/std::function

### Python Pattern
```python
class Animation:
    def __init__(self, rate_func=smooth):
        self.rate_func = rate_func  # Can be any callable

    def interpolate(self, t):
        alpha = self.rate_func(t)
        # ...
```

### C++ Solution
```cpp
class Animation {
    std::function<float(float)> rate_func;

public:
    Animation(std::function<float(float)> rf = smooth)
        : rate_func(std::move(rf)) {}

    void interpolate(float t) {
        float alpha = rate_func(t);
        // ...
    }
};

// Or use function pointers for zero overhead
using RateFunction = float(*)(float);

class Animation {
    RateFunction rate_func;

public:
    Animation(RateFunction rf = &smooth) : rate_func(rf) {}

    void interpolate(float t) {
        float alpha = rate_func(t);
        // ...
    }
};
```

---

## 8. Updaters (Callbacks) → std::function + Thread Safety

### Python Pattern
```python
class Mobject:
    def __init__(self):
        self.updaters = []

    def add_updater(self, func, call_with_dt=True):
        self.updaters.append((func, call_with_dt))

    def update(self, dt):
        for func, use_dt in self.updaters:
            if use_dt:
                func(self, dt)
            else:
                func(self)
```

### C++ Solution
```cpp
class Mobject {
public:
    using TimeUpdater = std::function<void(Mobject&, float)>;
    using NoTimeUpdater = std::function<void(Mobject&)>;

private:
    std::vector<TimeUpdater> time_updaters;
    std::vector<NoTimeUpdater> no_time_updaters;

    // Thread safety for concurrent updates
    mutable std::shared_mutex updater_mutex;

public:
    void add_updater(TimeUpdater func) {
        std::unique_lock lock(updater_mutex);
        time_updaters.push_back(std::move(func));
    }

    void add_updater(NoTimeUpdater func) {
        std::unique_lock lock(updater_mutex);
        no_time_updaters.push_back(std::move(func));
    }

    void update(float dt) {
        std::shared_lock lock(updater_mutex);

        for (auto& updater : time_updaters) {
            updater(*this, dt);
        }

        for (auto& updater : no_time_updaters) {
            updater(*this);
        }
    }
};
```

---

## 9. Metaclasses → Template Metaprogramming

### Python Pattern
```python
class ConvertToOpenGL(type):
    def __call__(cls, *args, **kwargs):
        if config.renderer == "opengl":
            return OpenGLMobject(*args, **kwargs)
        else:
            return Mobject(*args, **kwargs)

class Mobject(metaclass=ConvertToOpenGL):
    pass
```

### C++ Solution
```cpp
// Template-based factory
template<RendererType R>
struct MobjectFactory;

template<>
struct MobjectFactory<RendererType::Vulkan> {
    template<typename... Args>
    static auto create(Args&&... args) {
        return std::make_unique<VulkanMobject>(std::forward<Args>(args)...);
    }
};

// Or use constexpr if (C++17)
template<typename... Args>
auto create_mobject(Args&&... args) {
    if constexpr (config::renderer == RendererType::Vulkan) {
        return std::make_unique<VulkanMobject>(std::forward<Args>(args)...);
    } else {
        return std::make_unique<Mobject>(std::forward<Args>(args)...);
    }
}
```

**Note**: In C++ Manim, we're GPU-first, so this dual-mode approach is unnecessary.

---

## 10. Generator Functions → Coroutines (C++20)

### Python Pattern
```python
def interpolate_points(start, end, steps):
    for i in range(steps):
        yield start + (end - start) * (i / steps)

# Usage
for point in interpolate_points(p1, p2, 100):
    process(point)
```

### C++ Solution
```cpp
#include <generator>  // C++23

std::generator<glm::vec3> interpolate_points(
    const glm::vec3& start,
    const glm::vec3& end,
    int steps
) {
    for (int i = 0; i < steps; ++i) {
        float t = static_cast<float>(i) / steps;
        co_yield glm::mix(start, end, t);
    }
}

// Usage
for (const auto& point : interpolate_points(p1, p2, 100)) {
    process(point);
}
```

**Fallback for C++20** (without `std::generator`):
```cpp
// Custom generator class
template<typename T>
class Generator {
    std::function<std::optional<T>()> next_func;
    std::optional<T> current;

public:
    class Iterator {
        Generator* gen;
        std::optional<T> value;

    public:
        Iterator(Generator* g) : gen(g) {
            if (gen) value = gen->next();
        }

        T& operator*() { return *value; }
        Iterator& operator++() {
            value = gen->next();
            if (!value) gen = nullptr;
            return *this;
        }
        bool operator!=(const Iterator& other) const { return gen != other.gen; }
    };

    std::optional<T> next() { return next_func(); }
    Iterator begin() { return Iterator(this); }
    Iterator end() { return Iterator(nullptr); }
};
```

---

## 11. Context Managers → RAII

### Python Pattern
```python
with open("file.txt", "w") as f:
    f.write("data")
# File automatically closed

class GPUContext:
    def __enter__(self):
        self.init_gpu()
        return self

    def __exit__(self, *args):
        self.cleanup_gpu()

with GPUContext() as gpu:
    gpu.render()
# GPU automatically cleaned up
```

### C++ Solution
```cpp
// RAII (Resource Acquisition Is Initialization)
class File {
    std::FILE* fp;

public:
    File(const char* path, const char* mode) {
        fp = std::fopen(path, mode);
        if (!fp) throw std::runtime_error("Failed to open file");
    }

    ~File() {
        if (fp) std::fclose(fp);
    }

    // Prevent copying
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    // Allow moving
    File(File&& other) noexcept : fp(other.fp) {
        other.fp = nullptr;
    }

    void write(const std::string& data) {
        std::fwrite(data.data(), 1, data.size(), fp);
    }
};

// Usage
{
    File f("file.txt", "w");
    f.write("data");
}  // File automatically closed here

// GPU context
class VulkanContext {
    VkInstance instance;
    VkDevice device;

public:
    VulkanContext() {
        // Initialize Vulkan
        create_instance();
        create_device();
    }

    ~VulkanContext() {
        // Cleanup Vulkan
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    void render() { /* ... */ }
};

// Usage
{
    VulkanContext ctx;
    ctx.render();
}  // Vulkan automatically cleaned up
```

---

## 12. Global Interpreter Lock (GIL) → True Parallelism

### Python Limitation
```python
# In Python, threading doesn't provide true parallelism for CPU-bound tasks
import threading

def update_mobject(mob):
    # Heavy computation
    mob.points = transform(mob.points)

threads = [threading.Thread(target=update_mobject, args=(m,)) for m in mobjects]
# GIL prevents true parallel execution
```

### C++ Solution
```cpp
#include <thread>
#include <execution>

// Option 1: Manual threading
void update_mobjects_parallel(std::span<Mobject*> mobjects) {
    std::vector<std::thread> threads;

    for (auto* mob : mobjects) {
        threads.emplace_back([mob]() {
            mob->update_points();  // True parallel execution
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Option 2: Thread pool (better)
class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop = false;

public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(queue_mutex);
                        cv.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<typename F>
    void enqueue(F&& f) {
        {
            std::unique_lock lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        cv.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock lock(queue_mutex);
            stop = true;
        }
        cv.notify_all();
        for (auto& worker : workers) {
            worker.join();
        }
    }
};

// Usage
ThreadPool pool(std::thread::hardware_concurrency());

for (auto* mob : mobjects) {
    pool.enqueue([mob] {
        mob->update_points();
    });
}

// Option 3: Parallel STL (C++17)
std::for_each(std::execution::par, mobjects.begin(), mobjects.end(),
    [](Mobject* mob) {
        mob->update_points();
    });
```

---

## 13. Memory Management: GC → Smart Pointers + RAII

### Python Pattern
```python
class Scene:
    def __init__(self):
        self.mobjects = []

    def add(self, mob):
        self.mobjects.append(mob)
        # Python GC handles cleanup automatically
```

### C++ Solution
```cpp
class Scene {
    // Option 1: Unique ownership (most common)
    std::vector<std::unique_ptr<Mobject>> mobjects;

public:
    void add(std::unique_ptr<Mobject> mob) {
        mobjects.push_back(std::move(mob));
    }

    // Option 2: Shared ownership (when multiple owners needed)
    std::vector<std::shared_ptr<Mobject>> shared_mobjects;

    void add_shared(std::shared_ptr<Mobject> mob) {
        shared_mobjects.push_back(mob);
    }

    // Option 3: Non-owning references (for temporary access)
    void render(std::span<Mobject*> mob_refs) {
        for (auto* mob : mob_refs) {
            mob->render();
        }
    }
};

// Usage
auto scene = Scene();
scene.add(std::make_unique<Circle>());
// Circle automatically deleted when scene is destroyed

// Shared ownership example
auto mob = std::make_shared<Square>();
scene1.add_shared(mob);
scene2.add_shared(mob);  // Both scenes share the same object
```

**Memory Safety Rules**:
1. **Use unique_ptr by default** (single ownership)
2. **Use shared_ptr only when necessary** (multiple ownership)
3. **Use raw pointers only for non-owning references** (observer pattern)
4. **Avoid manual new/delete**

---

## 14. Exception Handling: Pythonic → C++ RAII

### Python Pattern
```python
try:
    file = open("scene.json")
    data = json.load(file)
finally:
    file.close()
```

### C++ Solution
```cpp
// RAII automatically handles cleanup, even on exception
void load_scene(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }

    nlohmann::json data;
    file >> data;

    // Process data
    // ...

}  // file automatically closed, even if exception is thrown

// Or use std::optional for non-exceptional errors
std::optional<nlohmann::json> try_load_scene(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;  // No exception
    }

    nlohmann::json data;
    try {
        file >> data;
        return data;
    } catch (...) {
        return std::nullopt;
    }
}

// Usage
if (auto data = try_load_scene("scene.json")) {
    process(*data);
} else {
    // Handle error
}
```

---

## 15. String Formatting: f-strings → std::format (C++20)

### Python Pattern
```python
name = "Circle"
radius = 2.5
message = f"Creating {name} with radius {radius:.2f}"
```

### C++ Solution
```cpp
#include <format>

std::string name = "Circle";
float radius = 2.5f;
std::string message = std::format("Creating {} with radius {:.2f}", name, radius);

// Or use fmt library (backport of std::format)
#include <fmt/format.h>
std::string message = fmt::format("Creating {} with radius {:.2f}", name, radius);
```

---

## 16. Module System: import → Namespaces + Headers

### Python Pattern
```python
from manim import Scene, Circle, FadeIn
from manim.utils.color import RED, BLUE
```

### C++ Solution
```cpp
// In header file (manim/manim.hpp)
namespace manim {
    class Scene { /* ... */ };
    class Circle { /* ... */ };
    class FadeIn { /* ... */ };

    namespace colors {
        constexpr Color RED{1.0f, 0.0f, 0.0f, 1.0f};
        constexpr Color BLUE{0.0f, 0.0f, 1.0f, 1.0f};
    }
}

// Usage
#include <manim/manim.hpp>

using namespace manim;
using namespace manim::colors;

auto scene = Scene();
auto circle = Circle();
circle.set_color(RED);
```

**Module System (C++20)**:
```cpp
// manim.cppm
export module manim;

export namespace manim {
    class Scene { /* ... */ };
    class Circle { /* ... */ };
}

// Usage
import manim;

auto scene = manim::Scene();
```

---

## 17. Operator Overloading: Rich Comparison → Spaceship Operator

### Python Pattern
```python
class Color:
    def __eq__(self, other):
        return self.r == other.r and self.g == other.g and self.b == other.b

    def __lt__(self, other):
        return self.luminance() < other.luminance()
```

### C++ Solution
```cpp
class Color {
    float r, g, b, a;

public:
    // C++20 spaceship operator (generates all comparison operators)
    auto operator<=>(const Color& other) const {
        return std::tie(r, g, b, a) <=> std::tie(other.r, other.g, other.b, other.a);
    }

    bool operator==(const Color& other) const = default;

    // Custom comparison based on luminance
    bool is_brighter_than(const Color& other) const {
        return luminance() > other.luminance();
    }

private:
    float luminance() const {
        return 0.299f * r + 0.587f * g + 0.114f * b;
    }
};
```

---

## 18. Configuration: Global State → Dependency Injection

### Python Pattern
```python
# Global config
config = {
    "frame_width": 16.0,
    "frame_height": 9.0,
    "fps": 60
}

class Scene:
    def __init__(self):
        self.width = config["frame_width"]
```

### C++ Solution
```cpp
// Option 1: Singleton (if truly global)
class Config {
    static Config* instance;

    float frame_width = 16.0f;
    float frame_height = 9.0f;
    int fps = 60;

    Config() = default;

public:
    static Config& get() {
        if (!instance) instance = new Config();
        return *instance;
    }

    float get_frame_width() const { return frame_width; }
    void set_frame_width(float w) { frame_width = w; }
};

// Option 2: Dependency injection (preferred for testability)
struct SceneConfig {
    float frame_width = 16.0f;
    float frame_height = 9.0f;
    int fps = 60;
};

class Scene {
    const SceneConfig& config;

public:
    Scene(const SceneConfig& cfg) : config(cfg) {}

    void setup() {
        float width = config.frame_width;
        // ...
    }
};

// Usage
SceneConfig config;
config.frame_width = 16.0f;

Scene scene(config);
```

---

## 19. Lazy Evaluation → std::lazy or Deferred Execution

### Python Pattern
```python
class Mobject:
    @property
    def bounding_box(self):
        # Computed on access
        return compute_bounding_box(self.points)
```

### C++ Solution
```cpp
class Mobject {
    mutable std::optional<BoundingBox> cached_bbox;

public:
    const BoundingBox& get_bounding_box() const {
        if (!cached_bbox) {
            cached_bbox = compute_bounding_box(points);
        }
        return *cached_bbox;
    }

    void invalidate_bbox() {
        cached_bbox.reset();
    }

    void update_points() {
        // ...
        invalidate_bbox();
    }
};
```

---

## 20. GPU Computation: Not in Python → Compute Shaders / CUDA

### Python Limitation
```python
# Python: CPU-bound NumPy operations
points = np.random.rand(1000000, 3)
transformed = points @ transformation_matrix.T  # Slow on CPU
```

### C++ GPU Solution
```cpp
// Compute shader (GLSL)
#version 450

layout(local_size_x = 256) in;

layout(std430, binding = 0) buffer Points {
    vec3 points[];
};

layout(binding = 1) uniform Transform {
    mat4 matrix;
};

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx < points.length()) {
        vec4 p = vec4(points[idx], 1.0);
        points[idx] = (matrix * p).xyz;
    }
}

// C++ host code
void transform_points_gpu(
    VkCommandBuffer cmd,
    VkBuffer points_buffer,
    const glm::mat4& matrix,
    uint32_t num_points
) {
    // Upload matrix to uniform buffer
    upload_uniform(matrix);

    // Dispatch compute shader
    uint32_t workgroups = (num_points + 255) / 256;
    vkCmdDispatchCompute(cmd, workgroups, 1, 1);

    // Memory barrier
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        0, 1, &barrier, 0, nullptr, 0, nullptr
    );
}

// 1,000,000 points transformed in <1ms on GPU vs ~100ms on CPU
```

---

## Summary Table

| Python Pattern | C++ Equivalent | Performance Impact |
|----------------|----------------|-------------------|
| Dynamic typing | Concepts/Templates | ✅ Compile-time checks |
| Duck typing | Variant/Virtual | ✅ Type safety |
| List comprehensions | Ranges (C++20) | ✅ Lazy evaluation |
| NumPy | Eigen/GLM + GPU | 🚀 100-1000x faster (GPU) |
| Property decorators | Getters/setters | ⚡ Zero overhead |
| Multiple inheritance | Composition/CRTP | ✅ Better design |
| Lambdas/callbacks | std::function | ⚡ Low overhead |
| Generators | Coroutines (C++20) | ✅ Memory efficient |
| Context managers | RAII | ✅ Automatic cleanup |
| GIL threading | True threads/Parallel STL | 🚀 True parallelism |
| Garbage collection | Smart pointers | ⚡ Deterministic |
| f-strings | std::format (C++20) | ✅ Type-safe |
| import | Modules (C++20) | ⚡ Faster compilation |
| Global config | Singleton/DI | ✅ Testable |
| Lazy evaluation | std::optional + cache | ⚡ On-demand computation |
| CPU-only | Compute shaders/CUDA | 🚀 100-1000x faster |

**Legend**:
- ✅ Better design/safety
- ⚡ Better performance (2-10x)
- 🚀 Dramatically better performance (100-1000x)

---

## Next Steps
See `ARCHITECTURE_DESIGN.md` for GPU-first architecture leveraging these patterns.

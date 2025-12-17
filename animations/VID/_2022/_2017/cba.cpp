// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2017/cba.py
//
// This is a stub port from Manim (Python) to Catalyst (C++).
// Fill in each scene function and replace TODOs with real Catalyst calls.

#include <Catalyst>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace {
constexpr uint32_t kWidth = 1920;
constexpr uint32_t kHeight = 1080;
constexpr float kManimFrameHeight = 8.0f;
constexpr float kManimUnit = static_cast<float>(kHeight) / kManimFrameHeight;  // px per Manim unit

constexpr float X(float manimX) { return static_cast<float>(kWidth) * 0.5f + manimX * kManimUnit; }
constexpr float Y(float manimY) { return static_cast<float>(kHeight) * 0.5f + manimY * kManimUnit; }
}  // namespace

static void EnumerableSaveScene(Catalyst& window) {
    // TODO: Port `EnumerableSaveScene.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/cba.py
    (void)window;
}

static void LayersOfAbstraction(Catalyst& window) {
    // TODO: Port `LayersOfAbstraction.construct()` (base: EnumerableSaveScene)
    // From: animations/VID/_2022/_2017/cba.py
    (void)window;
}

static void DifferenceOfSquares(Catalyst& window) {
    // TODO: Port `DifferenceOfSquares.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/cba.py
    (void)window;
}

static void Lightbulbs(Catalyst& window) {
    // TODO: Port `Lightbulbs.construct()` (base: EnumerableSaveScene)
    // From: animations/VID/_2022/_2017/cba.py
    (void)window;
}

static void LayersOfLightbulbs(Catalyst& window) {
    // TODO: Port `LayersOfLightbulbs.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/cba.py
    (void)window;
}

static void Test(Catalyst& window) {
    // TODO: Port `Test.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/cba.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"EnumerableSaveScene", &EnumerableSaveScene},
    {"LayersOfAbstraction", &LayersOfAbstraction},
    {"DifferenceOfSquares", &DifferenceOfSquares},
    {"Lightbulbs", &Lightbulbs},
    {"LayersOfLightbulbs", &LayersOfLightbulbs},
    {"Test", &Test},
};

int main(int argc, char** argv) {
    Catalyst window(kWidth, kHeight);
    window.setBackground("#000000");

    const char* chosen = (argc >= 2) ? argv[1] : kScenes[0].name;
    for (const auto& s : kScenes) {
        if (std::strcmp(s.name, chosen) == 0) {
            s.fn(window);
            window.run();
            return 0;
        }
    }

    // Unknown scene name; run the first scene.
    kScenes[0].fn(window);
    window.run();
    return 0;
}

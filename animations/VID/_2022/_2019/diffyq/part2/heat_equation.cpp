// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2019/diffyq/part2/heat_equation.py
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

static void TwoDBodyWithManyTemperatures(Catalyst& window) {
    // TODO: Port `TwoDBodyWithManyTemperatures.construct()` (base: ThreeDScene)
    // From: animations/VID/_2022/_2019/diffyq/part2/heat_equation.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void TwoDBodyWithManyTemperaturesGraph(Catalyst& window) {
    // TODO: Port `TwoDBodyWithManyTemperaturesGraph.construct()` (base: ExternallyAnimatedScene)
    // From: animations/VID/_2022/_2019/diffyq/part2/heat_equation.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void TwoDBodyWithManyTemperaturesContour(Catalyst& window) {
    // TODO: Port `TwoDBodyWithManyTemperaturesContour.construct()` (base: ExternallyAnimatedScene)
    // From: animations/VID/_2022/_2019/diffyq/part2/heat_equation.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void BringTwoRodsTogether(Catalyst& window) {
    // TODO: Port `BringTwoRodsTogether.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part2/heat_equation.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void TalkThrough1DHeatGraph(Catalyst& window) {
    // TODO: Port `TalkThrough1DHeatGraph.construct()` (base: ShowEvolvingTempGraphWithArrows)
    // From: animations/VID/_2022/_2019/diffyq/part2/heat_equation.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void ShowNewtonsLawGraph(Catalyst& window) {
    // TODO: Port `ShowNewtonsLawGraph.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part2/heat_equation.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"TwoDBodyWithManyTemperatures", &TwoDBodyWithManyTemperatures},
    {"TwoDBodyWithManyTemperaturesGraph", &TwoDBodyWithManyTemperaturesGraph},
    {"TwoDBodyWithManyTemperaturesContour", &TwoDBodyWithManyTemperaturesContour},
    {"BringTwoRodsTogether", &BringTwoRodsTogether},
    {"TalkThrough1DHeatGraph", &TalkThrough1DHeatGraph},
    {"ShowNewtonsLawGraph", &ShowNewtonsLawGraph},
};

int main(int argc, char** argv) {
    Catalyst window(kWidth, kHeight);
    window.setBackground("#000000");
    // window.set3DMode(true);

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

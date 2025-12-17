// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2019/diffyq/part2/wordy_scenes.py
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

static void WriteHeatEquationTemplate(Catalyst& window) {
    // TODO: Port `WriteHeatEquationTemplate.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part2/wordy_scenes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void BringTogether(Catalyst& window) {
    // TODO: Port `BringTogether.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part2/wordy_scenes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void CompareODEToPDE(Catalyst& window) {
    // TODO: Port `CompareODEToPDE.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part2/wordy_scenes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void TodaysTargetWrapper(Catalyst& window) {
    // TODO: Port `TodaysTargetWrapper.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part2/wordy_scenes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void TwoGraphTypeTitles(Catalyst& window) {
    // TODO: Port `TwoGraphTypeTitles.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part2/wordy_scenes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void ShowPartialDerivativeSymbols(Catalyst& window) {
    // TODO: Port `ShowPartialDerivativeSymbols.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part2/wordy_scenes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void AltHeatRHS(Catalyst& window) {
    // TODO: Port `AltHeatRHS.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part2/wordy_scenes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"WriteHeatEquationTemplate", &WriteHeatEquationTemplate},
    {"BringTogether", &BringTogether},
    {"CompareODEToPDE", &CompareODEToPDE},
    {"TodaysTargetWrapper", &TodaysTargetWrapper},
    {"TwoGraphTypeTitles", &TwoGraphTypeTitles},
    {"ShowPartialDerivativeSymbols", &ShowPartialDerivativeSymbols},
    {"AltHeatRHS", &AltHeatRHS},
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

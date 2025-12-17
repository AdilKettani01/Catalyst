// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2016/brachistochrone/cycloid.py
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

static void CycloidScene(Catalyst& window) {
    // TODO: Port `CycloidScene.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/brachistochrone/cycloid.py
    (void)window;
}

static void IntroduceCycloid(Catalyst& window) {
    // TODO: Port `IntroduceCycloid.construct()` (base: CycloidScene)
    // From: animations/VID/_2022/_2016/brachistochrone/cycloid.py
    (void)window;
}

static void LeviSolution(Catalyst& window) {
    // TODO: Port `LeviSolution.construct()` (base: CycloidScene)
    // From: animations/VID/_2022/_2016/brachistochrone/cycloid.py
    (void)window;
}

static void EquationsForCycloid(Catalyst& window) {
    // TODO: Port `EquationsForCycloid.construct()` (base: CycloidScene)
    // From: animations/VID/_2022/_2016/brachistochrone/cycloid.py
    (void)window;
}

static void SlidingObject(Catalyst& window) {
    // TODO: Port `SlidingObject.construct()` (base: CycloidScene)
    // From: animations/VID/_2022/_2016/brachistochrone/cycloid.py
    (void)window;
}

static void RotateWheel(Catalyst& window) {
    // TODO: Port `RotateWheel.construct()` (base: CycloidScene)
    // From: animations/VID/_2022/_2016/brachistochrone/cycloid.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"CycloidScene", &CycloidScene},
    {"IntroduceCycloid", &IntroduceCycloid},
    {"LeviSolution", &LeviSolution},
    {"EquationsForCycloid", &EquationsForCycloid},
    {"SlidingObject", &SlidingObject},
    {"RotateWheel", &RotateWheel},
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

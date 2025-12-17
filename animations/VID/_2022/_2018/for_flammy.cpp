// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2018/for_flammy.py
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

static void MadAtMathologer(Catalyst& window) {
    // TODO: Port `MadAtMathologer.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2018/for_flammy.py
    (void)window;
}

static void JustTheIntegral(Catalyst& window) {
    // TODO: Port `JustTheIntegral.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/for_flammy.py
    (void)window;
}

static void SphereVideoWrapper(Catalyst& window) {
    // TODO: Port `SphereVideoWrapper.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/for_flammy.py
    (void)window;
}

static void IntegralSymbols(Catalyst& window) {
    // TODO: Port `IntegralSymbols.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/for_flammy.py
    (void)window;
}

static void ShamelessPlug(Catalyst& window) {
    // TODO: Port `ShamelessPlug.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/for_flammy.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"MadAtMathologer", &MadAtMathologer},
    {"JustTheIntegral", &JustTheIntegral},
    {"SphereVideoWrapper", &SphereVideoWrapper},
    {"IntegralSymbols", &IntegralSymbols},
    {"ShamelessPlug", &ShamelessPlug},
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

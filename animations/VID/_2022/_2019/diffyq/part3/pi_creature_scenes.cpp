// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2019/diffyq/part3/pi_creature_scenes.py
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

static void IveHeardOfThis(Catalyst& window) {
    // TODO: Port `IveHeardOfThis.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part3/pi_creature_scenes.py
    (void)window;
}

static void InFouriersShoes(Catalyst& window) {
    // TODO: Port `InFouriersShoes.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2019/diffyq/part3/pi_creature_scenes.py
    (void)window;
}

static void SineCurveIsUnrealistic(Catalyst& window) {
    // TODO: Port `SineCurveIsUnrealistic.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part3/pi_creature_scenes.py
    (void)window;
}

static void IfOnly(Catalyst& window) {
    // TODO: Port `IfOnly.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part3/pi_creature_scenes.py
    (void)window;
}

static void SoWeGotNowhere(Catalyst& window) {
    // TODO: Port `SoWeGotNowhere.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part3/pi_creature_scenes.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"IveHeardOfThis", &IveHeardOfThis},
    {"InFouriersShoes", &InFouriersShoes},
    {"SineCurveIsUnrealistic", &SineCurveIsUnrealistic},
    {"IfOnly", &IfOnly},
    {"SoWeGotNowhere", &SoWeGotNowhere},
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

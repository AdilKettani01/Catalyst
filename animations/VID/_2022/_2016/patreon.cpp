// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2016/patreon.py
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

static void SideGigToFullTime(Catalyst& window) {
    // TODO: Port `SideGigToFullTime.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void TakesTime(Catalyst& window) {
    // TODO: Port `TakesTime.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void GrowingToDoList(Catalyst& window) {
    // TODO: Port `GrowingToDoList.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void TwoTypesOfVideos(Catalyst& window) {
    // TODO: Port `TwoTypesOfVideos.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void ClassWatching(Catalyst& window) {
    // TODO: Port `ClassWatching.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void RandolphWatching(Catalyst& window) {
    // TODO: Port `RandolphWatching.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void RandolphWatchingWithLaptop(Catalyst& window) {
    // TODO: Port `RandolphWatchingWithLaptop.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void GrowRonaksSierpinski(Catalyst& window) {
    // TODO: Port `GrowRonaksSierpinski.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void PatreonLogo(Catalyst& window) {
    // TODO: Port `PatreonLogo.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void PatreonLogin(Catalyst& window) {
    // TODO: Port `PatreonLogin.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void PythagoreanTransformation(Catalyst& window) {
    // TODO: Port `PythagoreanTransformation.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void KindWordsOnEoLA(Catalyst& window) {
    // TODO: Port `KindWordsOnEoLA.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void MakeALotOfPiCreaturesHappy(Catalyst& window) {
    // TODO: Port `MakeALotOfPiCreaturesHappy.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void IntegrationByParts(Catalyst& window) {
    // TODO: Port `IntegrationByParts.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

static void EndScreen(Catalyst& window) {
    // TODO: Port `EndScreen.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2016/patreon.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"SideGigToFullTime", &SideGigToFullTime},
    {"TakesTime", &TakesTime},
    {"GrowingToDoList", &GrowingToDoList},
    {"TwoTypesOfVideos", &TwoTypesOfVideos},
    {"ClassWatching", &ClassWatching},
    {"RandolphWatching", &RandolphWatching},
    {"RandolphWatchingWithLaptop", &RandolphWatchingWithLaptop},
    {"GrowRonaksSierpinski", &GrowRonaksSierpinski},
    {"PatreonLogo", &PatreonLogo},
    {"PatreonLogin", &PatreonLogin},
    {"PythagoreanTransformation", &PythagoreanTransformation},
    {"KindWordsOnEoLA", &KindWordsOnEoLA},
    {"MakeALotOfPiCreaturesHappy", &MakeALotOfPiCreaturesHappy},
    {"IntegrationByParts", &IntegrationByParts},
    {"EndScreen", &EndScreen},
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

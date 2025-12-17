// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2025/guest_videos/misc_animations.py
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

static void WelchLabsIntroCard(Catalyst& window) {
    // TODO: Port `WelchLabsIntroCard.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/guest_videos/misc_animations.py
    (void)window;
}

static void SubManifolds(Catalyst& window) {
    // TODO: Port `SubManifolds.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/guest_videos/misc_animations.py
    (void)window;
}

static void ComposingFeatures(Catalyst& window) {
    // TODO: Port `ComposingFeatures.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/guest_videos/misc_animations.py
    (void)window;
}

static void IMOGoldOrganizations(Catalyst& window) {
    // TODO: Port `IMOGoldOrganizations.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/guest_videos/misc_animations.py
    (void)window;
}

static void SeriesOfFiveVideos(Catalyst& window) {
    // TODO: Port `SeriesOfFiveVideos.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/guest_videos/misc_animations.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"WelchLabsIntroCard", &WelchLabsIntroCard},
    {"SubManifolds", &SubManifolds},
    {"ComposingFeatures", &ComposingFeatures},
    {"IMOGoldOrganizations", &IMOGoldOrganizations},
    {"SeriesOfFiveVideos", &SeriesOfFiveVideos},
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

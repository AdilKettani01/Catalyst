// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/galois/art_supplements.py
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

static void AmbientPermutations(Catalyst& window) {
    // TODO: Port `AmbientPermutations.construct()` (base: Scene)
    // From: animations/VID/_2022/galois/art_supplements.py
    (void)window;
}

static void WriteName(Catalyst& window) {
    // TODO: Port `WriteName.construct()` (base: InteractiveScene)
    // From: animations/VID/_2022/galois/art_supplements.py
    (void)window;
}

static void TimelineTransition(Catalyst& window) {
    // TODO: Port `TimelineTransition.construct()` (base: InteractiveScene)
    // From: animations/VID/_2022/galois/art_supplements.py
    (void)window;
}

static void OutpaintTransition(Catalyst& window) {
    // TODO: Port `OutpaintTransition.construct()` (base: InteractiveScene)
    // From: animations/VID/_2022/galois/art_supplements.py
    (void)window;
}

static void NightSkyOutpaintingTransition(Catalyst& window) {
    // TODO: Port `NightSkyOutpaintingTransition.construct()` (base: InteractiveScene)
    // From: animations/VID/_2022/galois/art_supplements.py
    (void)window;
}

static void LastWordsQuote(Catalyst& window) {
    // TODO: Port `LastWordsQuote.construct()` (base: InteractiveScene)
    // From: animations/VID/_2022/galois/art_supplements.py
    (void)window;
}

static void InfamousCoquette(Catalyst& window) {
    // TODO: Port `InfamousCoquette.construct()` (base: InteractiveScene)
    // From: animations/VID/_2022/galois/art_supplements.py
    (void)window;
}

static void NightBeforeQuote(Catalyst& window) {
    // TODO: Port `NightBeforeQuote.construct()` (base: InteractiveScene)
    // From: animations/VID/_2022/galois/art_supplements.py
    (void)window;
}

static void CauchyFourierPoisson(Catalyst& window) {
    // TODO: Port `CauchyFourierPoisson.construct()` (base: InteractiveScene)
    // From: animations/VID/_2022/galois/art_supplements.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"AmbientPermutations", &AmbientPermutations},
    {"WriteName", &WriteName},
    {"TimelineTransition", &TimelineTransition},
    {"OutpaintTransition", &OutpaintTransition},
    {"NightSkyOutpaintingTransition", &NightSkyOutpaintingTransition},
    {"LastWordsQuote", &LastWordsQuote},
    {"InfamousCoquette", &InfamousCoquette},
    {"NightBeforeQuote", &NightBeforeQuote},
    {"CauchyFourierPoisson", &CauchyFourierPoisson},
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

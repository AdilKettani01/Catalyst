// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2024/antp/main.py
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

static void TwinPrimScrolling(Catalyst& window) {
    // TODO: Port `TwinPrimScrolling.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void Timeline(Catalyst& window) {
    // TODO: Port `Timeline.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void InfinitePrimes(Catalyst& window) {
    // TODO: Port `InfinitePrimes.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void ThoughtBubble(Catalyst& window) {
    // TODO: Port `ThoughtBubble.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void SimpleBubble(Catalyst& window) {
    // TODO: Port `SimpleBubble.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void EuclidProof(Catalyst& window) {
    // TODO: Port `EuclidProof.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void PrimeDensityHistogram(Catalyst& window) {
    // TODO: Port `PrimeDensityHistogram.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void PrimesNearMillion(Catalyst& window) {
    // TODO: Port `PrimesNearMillion.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void PrimePanning(Catalyst& window) {
    // TODO: Port `PrimePanning.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void SieveWithMod(Catalyst& window) {
    // TODO: Port `SieveWithMod.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void DensityFormula(Catalyst& window) {
    // TODO: Port `DensityFormula.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void OldGapsInPrimes(Catalyst& window) {
    // TODO: Port `OldGapsInPrimes.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void NewGapsInPrimes(Catalyst& window) {
    // TODO: Port `NewGapsInPrimes.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void CrankEmail(Catalyst& window) {
    // TODO: Port `CrankEmail.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void SieveOfEratosthenes(Catalyst& window) {
    // TODO: Port `SieveOfEratosthenes.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

static void WannaProve(Catalyst& window) {
    // TODO: Port `WannaProve.construct()` (base: InteractiveScene)
    // From: animations/VID/_2024/antp/main.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"TwinPrimScrolling", &TwinPrimScrolling},
    {"Timeline", &Timeline},
    {"InfinitePrimes", &InfinitePrimes},
    {"ThoughtBubble", &ThoughtBubble},
    {"SimpleBubble", &SimpleBubble},
    {"EuclidProof", &EuclidProof},
    {"PrimeDensityHistogram", &PrimeDensityHistogram},
    {"PrimesNearMillion", &PrimesNearMillion},
    {"PrimePanning", &PrimePanning},
    {"SieveWithMod", &SieveWithMod},
    {"DensityFormula", &DensityFormula},
    {"OldGapsInPrimes", &OldGapsInPrimes},
    {"NewGapsInPrimes", &NewGapsInPrimes},
    {"CrankEmail", &CrankEmail},
    {"SieveOfEratosthenes", &SieveOfEratosthenes},
    {"WannaProve", &WannaProve},
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

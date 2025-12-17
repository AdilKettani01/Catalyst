// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
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

static void SmallAngleApproximationTex(Catalyst& window) {
    // TODO: Port `SmallAngleApproximationTex.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void StrogatzQuote(Catalyst& window) {
    // TODO: Port `StrogatzQuote.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void WriteInRadians(Catalyst& window) {
    // TODO: Port `WriteInRadians.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void XEqLThetaToCorner(Catalyst& window) {
    // TODO: Port `XEqLThetaToCorner.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void ComingUp(Catalyst& window) {
    // TODO: Port `ComingUp.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void InputLabel(Catalyst& window) {
    // TODO: Port `InputLabel.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void ReallyHardToSolve(Catalyst& window) {
    // TODO: Port `ReallyHardToSolve.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void ReasonForSolution(Catalyst& window) {
    // TODO: Port `ReasonForSolution.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void WritePhaseSpace(Catalyst& window) {
    // TODO: Port `WritePhaseSpace.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void GleickQuote(Catalyst& window) {
    // TODO: Port `GleickQuote.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void WritePhaseFlow(Catalyst& window) {
    // TODO: Port `WritePhaseFlow.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void ShowSineValues(Catalyst& window) {
    // TODO: Port `ShowSineValues.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void SetAsideSeekingSolution(Catalyst& window) {
    // TODO: Port `SetAsideSeekingSolution.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void ThreeBodyTitle(Catalyst& window) {
    // TODO: Port `ThreeBodyTitle.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void ThreeBodySymbols(Catalyst& window) {
    // TODO: Port `ThreeBodySymbols.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void ThreeBodyEquation(Catalyst& window) {
    // TODO: Port `ThreeBodyEquation.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void JumpToThisPoint(Catalyst& window) {
    // TODO: Port `JumpToThisPoint.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void ChaosTitle(Catalyst& window) {
    // TODO: Port `ChaosTitle.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

static void RevisitQuote(Catalyst& window) {
    // TODO: Port `RevisitQuote.construct()` (base: StrogatzQuote)
    // From: animations/VID/_2022/_2019/diffyq/part1/wordy_scenes.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"SmallAngleApproximationTex", &SmallAngleApproximationTex},
    {"StrogatzQuote", &StrogatzQuote},
    {"WriteInRadians", &WriteInRadians},
    {"XEqLThetaToCorner", &XEqLThetaToCorner},
    {"ComingUp", &ComingUp},
    {"InputLabel", &InputLabel},
    {"ReallyHardToSolve", &ReallyHardToSolve},
    {"ReasonForSolution", &ReasonForSolution},
    {"WritePhaseSpace", &WritePhaseSpace},
    {"GleickQuote", &GleickQuote},
    {"WritePhaseFlow", &WritePhaseFlow},
    {"ShowSineValues", &ShowSineValues},
    {"SetAsideSeekingSolution", &SetAsideSeekingSolution},
    {"ThreeBodyTitle", &ThreeBodyTitle},
    {"ThreeBodySymbols", &ThreeBodySymbols},
    {"ThreeBodyEquation", &ThreeBodyEquation},
    {"JumpToThisPoint", &JumpToThisPoint},
    {"ChaosTitle", &ChaosTitle},
    {"RevisitQuote", &RevisitQuote},
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

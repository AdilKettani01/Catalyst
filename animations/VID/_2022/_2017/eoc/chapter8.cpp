// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2017/eoc/chapter8.py
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

static void Chapter8OpeningQuote(Catalyst& window) {
    // TODO: Port `Chapter8OpeningQuote.construct()` (base: OpeningQuote)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void ThisVideo(Catalyst& window) {
    // TODO: Port `ThisVideo.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void PlotVelocity(Catalyst& window) {
    // TODO: Port `PlotVelocity.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void Chapter2Wrapper(Catalyst& window) {
    // TODO: Port `Chapter2Wrapper.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void Antiderivative(Catalyst& window) {
    // TODO: Port `Antiderivative.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void ConstantVelocityCar(Catalyst& window) {
    // TODO: Port `ConstantVelocityCar.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void PiecewiseConstantCar(Catalyst& window) {
    // TODO: Port `PiecewiseConstantCar.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void DontKnowHowToHandleNonConstant(Catalyst& window) {
    // TODO: Port `DontKnowHowToHandleNonConstant.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void CarJourneyApproximation(Catalyst& window) {
    // TODO: Port `CarJourneyApproximation.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void TellMeThatsNotSurprising(Catalyst& window) {
    // TODO: Port `TellMeThatsNotSurprising.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void HowDoesThisHelp(Catalyst& window) {
    // TODO: Port `HowDoesThisHelp.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void AreaUnderACurve(Catalyst& window) {
    // TODO: Port `AreaUnderACurve.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void AreaIsDerivative(Catalyst& window) {
    // TODO: Port `AreaIsDerivative.construct()` (base: PlotVelocity)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void DirectInterpretationOfDsDt(Catalyst& window) {
    // TODO: Port `DirectInterpretationOfDsDt.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void FundamentalTheorem(Catalyst& window) {
    // TODO: Port `FundamentalTheorem.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void LetsRecap(Catalyst& window) {
    // TODO: Port `LetsRecap.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void NegativeArea(Catalyst& window) {
    // TODO: Port `NegativeArea.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

static void NextVideo(Catalyst& window) {
    // TODO: Port `NextVideo.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter8.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"Chapter8OpeningQuote", &Chapter8OpeningQuote},
    {"ThisVideo", &ThisVideo},
    {"PlotVelocity", &PlotVelocity},
    {"Chapter2Wrapper", &Chapter2Wrapper},
    {"Antiderivative", &Antiderivative},
    {"ConstantVelocityCar", &ConstantVelocityCar},
    {"PiecewiseConstantCar", &PiecewiseConstantCar},
    {"DontKnowHowToHandleNonConstant", &DontKnowHowToHandleNonConstant},
    {"CarJourneyApproximation", &CarJourneyApproximation},
    {"TellMeThatsNotSurprising", &TellMeThatsNotSurprising},
    {"HowDoesThisHelp", &HowDoesThisHelp},
    {"AreaUnderACurve", &AreaUnderACurve},
    {"AreaIsDerivative", &AreaIsDerivative},
    {"DirectInterpretationOfDsDt", &DirectInterpretationOfDsDt},
    {"FundamentalTheorem", &FundamentalTheorem},
    {"LetsRecap", &LetsRecap},
    {"NegativeArea", &NegativeArea},
    {"NextVideo", &NextVideo},
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

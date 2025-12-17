// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2018/pi_day.py
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

static void CircleConstants(Catalyst& window) {
    // TODO: Port `CircleConstants.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void AnalysisQuote(Catalyst& window) {
    // TODO: Port `AnalysisQuote.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void BernoulliQuote(Catalyst& window) {
    // TODO: Port `BernoulliQuote.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void EulerSignature(Catalyst& window) {
    // TODO: Port `EulerSignature.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void PiTauDebate(Catalyst& window) {
    // TODO: Port `PiTauDebate.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void HartlAndPalais(Catalyst& window) {
    // TODO: Port `HartlAndPalais.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void ManyFormulas(Catalyst& window) {
    // TODO: Port `ManyFormulas.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void HistoryOfOurPeople(Catalyst& window) {
    // TODO: Port `HistoryOfOurPeople.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void TauFalls(Catalyst& window) {
    // TODO: Port `TauFalls.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void EulerWrites628(Catalyst& window) {
    // TODO: Port `EulerWrites628.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void HeroAndVillain(Catalyst& window) {
    // TODO: Port `HeroAndVillain.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void AnalysisQuote(Catalyst& window) {
    // TODO: Port `AnalysisQuote.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void QuarterTurn(Catalyst& window) {
    // TODO: Port `QuarterTurn.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void UsingTheta(Catalyst& window) {
    // TODO: Port `UsingTheta.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void ThingsNamedAfterEuler(Catalyst& window) {
    // TODO: Port `ThingsNamedAfterEuler.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void EulerThinking(Catalyst& window) {
    // TODO: Port `EulerThinking.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void WhatIsRight(Catalyst& window) {
    // TODO: Port `WhatIsRight.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void AskPuzzle(Catalyst& window) {
    // TODO: Port `AskPuzzle.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void ChangeTopic(Catalyst& window) {
    // TODO: Port `ChangeTopic.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void SpecialThanks(Catalyst& window) {
    // TODO: Port `SpecialThanks.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

static void Thumbnail(Catalyst& window) {
    // TODO: Port `Thumbnail.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/pi_day.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"CircleConstants", &CircleConstants},
    {"AnalysisQuote", &AnalysisQuote},
    {"BernoulliQuote", &BernoulliQuote},
    {"EulerSignature", &EulerSignature},
    {"PiTauDebate", &PiTauDebate},
    {"HartlAndPalais", &HartlAndPalais},
    {"ManyFormulas", &ManyFormulas},
    {"HistoryOfOurPeople", &HistoryOfOurPeople},
    {"TauFalls", &TauFalls},
    {"EulerWrites628", &EulerWrites628},
    {"HeroAndVillain", &HeroAndVillain},
    {"AnalysisQuote", &AnalysisQuote},
    {"QuarterTurn", &QuarterTurn},
    {"UsingTheta", &UsingTheta},
    {"ThingsNamedAfterEuler", &ThingsNamedAfterEuler},
    {"EulerThinking", &EulerThinking},
    {"WhatIsRight", &WhatIsRight},
    {"AskPuzzle", &AskPuzzle},
    {"ChangeTopic", &ChangeTopic},
    {"SpecialThanks", &SpecialThanks},
    {"Thumbnail", &Thumbnail},
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

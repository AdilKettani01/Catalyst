// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
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

static void ThreeMainObservations(Catalyst& window) {
    // TODO: Port `ThreeMainObservations.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

static void LastChapterWrapper(Catalyst& window) {
    // TODO: Port `LastChapterWrapper.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

static void BorderRect(Catalyst& window) {
    // TODO: Port `BorderRect.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

static void SeekIdealized(Catalyst& window) {
    // TODO: Port `SeekIdealized.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

static void SecondDerivativeOfSine(Catalyst& window) {
    // TODO: Port `SecondDerivativeOfSine.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

static void ExpVideoWrapper(Catalyst& window) {
    // TODO: Port `ExpVideoWrapper.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

static void FlatAtBoundaryWords(Catalyst& window) {
    // TODO: Port `FlatAtBoundaryWords.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

static void WriteOutBoundaryCondition(Catalyst& window) {
    // TODO: Port `WriteOutBoundaryCondition.construct()` (base: FlatAtBoundaryWords)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

static void CompareFreqDecays1to2(Catalyst& window) {
    // TODO: Port `CompareFreqDecays1to2.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

static void WorryAboutGenerality(Catalyst& window) {
    // TODO: Port `WorryAboutGenerality.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part3/wordy_scenes.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"ThreeMainObservations", &ThreeMainObservations},
    {"LastChapterWrapper", &LastChapterWrapper},
    {"BorderRect", &BorderRect},
    {"SeekIdealized", &SeekIdealized},
    {"SecondDerivativeOfSine", &SecondDerivativeOfSine},
    {"ExpVideoWrapper", &ExpVideoWrapper},
    {"FlatAtBoundaryWords", &FlatAtBoundaryWords},
    {"WriteOutBoundaryCondition", &WriteOutBoundaryCondition},
    {"CompareFreqDecays1to2", &CompareFreqDecays1to2},
    {"WorryAboutGenerality", &WorryAboutGenerality},
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

// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2018/eop/bayes.py
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

static void IntroducePokerHand(Catalyst& window) {
    // TODO: Port `IntroducePokerHand.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void HowDoesPokerWork(Catalyst& window) {
    // TODO: Port `HowDoesPokerWork.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void YourGutKnowsBayesRule(Catalyst& window) {
    // TODO: Port `YourGutKnowsBayesRule.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void UpdatePokerPrior(Catalyst& window) {
    // TODO: Port `UpdatePokerPrior.construct()` (base: SampleSpaceScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void BayesRuleInMemory(Catalyst& window) {
    // TODO: Port `BayesRuleInMemory.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void NextVideoWrapper(Catalyst& window) {
    // TODO: Port `NextVideoWrapper.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void BayesianNetworkPreview(Catalyst& window) {
    // TODO: Port `BayesianNetworkPreview.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void GeneralizeBayesRule(Catalyst& window) {
    // TODO: Port `GeneralizeBayesRule.construct()` (base: SampleSpaceScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void MoreExamples(Catalyst& window) {
    // TODO: Port `MoreExamples.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void MusicExample(Catalyst& window) {
    // TODO: Port `MusicExample.construct()` (base: SampleSpaceScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void FinalWordsOnRule(Catalyst& window) {
    // TODO: Port `FinalWordsOnRule.construct()` (base: SampleSpaceScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void Thumbnail(Catalyst& window) {
    // TODO: Port `Thumbnail.construct()` (base: SampleSpaceScene)
    // From: animations/VID/_2022/_2018/eop/bayes.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"IntroducePokerHand", &IntroducePokerHand},
    {"HowDoesPokerWork", &HowDoesPokerWork},
    {"YourGutKnowsBayesRule", &YourGutKnowsBayesRule},
    {"UpdatePokerPrior", &UpdatePokerPrior},
    {"BayesRuleInMemory", &BayesRuleInMemory},
    {"NextVideoWrapper", &NextVideoWrapper},
    {"BayesianNetworkPreview", &BayesianNetworkPreview},
    {"GeneralizeBayesRule", &GeneralizeBayesRule},
    {"MoreExamples", &MoreExamples},
    {"MusicExample", &MusicExample},
    {"FinalWordsOnRule", &FinalWordsOnRule},
    {"Thumbnail", &Thumbnail},
};

int main(int argc, char** argv) {
    Catalyst window(kWidth, kHeight);
    window.setBackground("#000000");
    // window.set3DMode(true);

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

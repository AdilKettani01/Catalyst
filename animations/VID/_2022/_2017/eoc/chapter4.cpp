// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2017/eoc/chapter4.py
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

static void TransitionFromLastVideo(Catalyst& window) {
    // TODO: Port `TransitionFromLastVideo.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void DampenedSpring(Catalyst& window) {
    // TODO: Port `DampenedSpring.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void ComingUp(Catalyst& window) {
    // TODO: Port `ComingUp.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void PreSumRuleDiscussion(Catalyst& window) {
    // TODO: Port `PreSumRuleDiscussion.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void SumRule(Catalyst& window) {
    // TODO: Port `SumRule.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void DiscussProducts(Catalyst& window) {
    // TODO: Port `DiscussProducts.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void NotGraphsForProducts(Catalyst& window) {
    // TODO: Port `NotGraphsForProducts.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void ConfusedMorty(Catalyst& window) {
    // TODO: Port `ConfusedMorty.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void IntroduceProductAsArea(Catalyst& window) {
    // TODO: Port `IntroduceProductAsArea.construct()` (base: ReconfigurableScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void WriteDXSquared(Catalyst& window) {
    // TODO: Port `WriteDXSquared.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void MneumonicExample(Catalyst& window) {
    // TODO: Port `MneumonicExample.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void ConstantMultiplication(Catalyst& window) {
    // TODO: Port `ConstantMultiplication.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void ShoveXSquaredInSine(Catalyst& window) {
    // TODO: Port `ShoveXSquaredInSine.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void ThreeLinesChainRule(Catalyst& window) {
    // TODO: Port `ThreeLinesChainRule.construct()` (base: ReconfigurableScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void GeneralizeChainRule(Catalyst& window) {
    // TODO: Port `GeneralizeChainRule.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void WatchingVideo(Catalyst& window) {
    // TODO: Port `WatchingVideo.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

static void NextVideo(Catalyst& window) {
    // TODO: Port `NextVideo.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter4.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"TransitionFromLastVideo", &TransitionFromLastVideo},
    {"DampenedSpring", &DampenedSpring},
    {"ComingUp", &ComingUp},
    {"PreSumRuleDiscussion", &PreSumRuleDiscussion},
    {"SumRule", &SumRule},
    {"DiscussProducts", &DiscussProducts},
    {"NotGraphsForProducts", &NotGraphsForProducts},
    {"ConfusedMorty", &ConfusedMorty},
    {"IntroduceProductAsArea", &IntroduceProductAsArea},
    {"WriteDXSquared", &WriteDXSquared},
    {"MneumonicExample", &MneumonicExample},
    {"ConstantMultiplication", &ConstantMultiplication},
    {"ShoveXSquaredInSine", &ShoveXSquaredInSine},
    {"ThreeLinesChainRule", &ThreeLinesChainRule},
    {"GeneralizeChainRule", &GeneralizeChainRule},
    {"WatchingVideo", &WatchingVideo},
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

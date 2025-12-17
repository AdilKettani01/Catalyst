// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2017/nn/part3.py
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

static void LayOutPlan(Catalyst& window) {
    // TODO: Port `LayOutPlan.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void GetLostInNotation(Catalyst& window) {
    // TODO: Port `GetLostInNotation.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void FocusOnOneExample(Catalyst& window) {
    // TODO: Port `FocusOnOneExample.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void WriteHebbian(Catalyst& window) {
    // TODO: Port `WriteHebbian.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void NotANeuroScientist(Catalyst& window) {
    // TODO: Port `NotANeuroScientist.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void ConstructGradientFromAllTrainingExamples(Catalyst& window) {
    // TODO: Port `ConstructGradientFromAllTrainingExamples.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void WatchPreviousScene(Catalyst& window) {
    // TODO: Port `WatchPreviousScene.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void OpenCloseSGD(Catalyst& window) {
    // TODO: Port `OpenCloseSGD.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void OrganizeDataIntoMiniBatches(Catalyst& window) {
    // TODO: Port `OrganizeDataIntoMiniBatches.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void SGDSteps(Catalyst& window) {
    // TODO: Port `SGDSteps.construct()` (base: ExternallyAnimatedScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void GradientDescentSteps(Catalyst& window) {
    // TODO: Port `GradientDescentSteps.construct()` (base: ExternallyAnimatedScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void SwimmingInTerms(Catalyst& window) {
    // TODO: Port `SwimmingInTerms.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void BackpropCode(Catalyst& window) {
    // TODO: Port `BackpropCode.construct()` (base: ExternallyAnimatedScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void BackpropCodeAddOn(Catalyst& window) {
    // TODO: Port `BackpropCodeAddOn.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void CannotFollowCode(Catalyst& window) {
    // TODO: Port `CannotFollowCode.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void EOCWrapper(Catalyst& window) {
    // TODO: Port `EOCWrapper.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void IsntThatOverSimplified(Catalyst& window) {
    // TODO: Port `IsntThatOverSimplified.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void ThatsPrettyMuchIt(Catalyst& window) {
    // TODO: Port `ThatsPrettyMuchIt.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void PatYourselfOnTheBack(Catalyst& window) {
    // TODO: Port `PatYourselfOnTheBack.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void ThatsALotToThinkAbout(Catalyst& window) {
    // TODO: Port `ThatsALotToThinkAbout.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void LayersOfComplexity(Catalyst& window) {
    // TODO: Port `LayersOfComplexity.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

static void SponsorFrame(Catalyst& window) {
    // TODO: Port `SponsorFrame.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/nn/part3.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"LayOutPlan", &LayOutPlan},
    {"GetLostInNotation", &GetLostInNotation},
    {"FocusOnOneExample", &FocusOnOneExample},
    {"WriteHebbian", &WriteHebbian},
    {"NotANeuroScientist", &NotANeuroScientist},
    {"ConstructGradientFromAllTrainingExamples", &ConstructGradientFromAllTrainingExamples},
    {"WatchPreviousScene", &WatchPreviousScene},
    {"OpenCloseSGD", &OpenCloseSGD},
    {"OrganizeDataIntoMiniBatches", &OrganizeDataIntoMiniBatches},
    {"SGDSteps", &SGDSteps},
    {"GradientDescentSteps", &GradientDescentSteps},
    {"SwimmingInTerms", &SwimmingInTerms},
    {"BackpropCode", &BackpropCode},
    {"BackpropCodeAddOn", &BackpropCodeAddOn},
    {"CannotFollowCode", &CannotFollowCode},
    {"EOCWrapper", &EOCWrapper},
    {"IsntThatOverSimplified", &IsntThatOverSimplified},
    {"ThatsPrettyMuchIt", &ThatsPrettyMuchIt},
    {"PatYourselfOnTheBack", &PatYourselfOnTheBack},
    {"ThatsALotToThinkAbout", &ThatsALotToThinkAbout},
    {"LayersOfComplexity", &LayersOfComplexity},
    {"SponsorFrame", &SponsorFrame},
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

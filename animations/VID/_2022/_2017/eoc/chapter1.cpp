// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2017/eoc/chapter1.py
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

static void Eoc1Thumbnail(Catalyst& window) {
    // TODO: Port `Eoc1Thumbnail.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void CircleScene(Catalyst& window) {
    // TODO: Port `CircleScene.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void Introduction(Catalyst& window) {
    // TODO: Port `Introduction.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void PreviewFrame(Catalyst& window) {
    // TODO: Port `PreviewFrame.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void ProductRuleDiagram(Catalyst& window) {
    // TODO: Port `ProductRuleDiagram.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void IntroduceCircle(Catalyst& window) {
    // TODO: Port `IntroduceCircle.construct()` (base: CircleScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void ApproximateOneRing(Catalyst& window) {
    // TODO: Port `ApproximateOneRing.construct()` (base: CircleScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void MoveForwardWithApproximation(Catalyst& window) {
    // TODO: Port `MoveForwardWithApproximation.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void GraphRectangles(Catalyst& window) {
    // TODO: Port `GraphRectangles.construct()` (base: CircleScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void ThinkLikeAMathematician(Catalyst& window) {
    // TODO: Port `ThinkLikeAMathematician.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void TwoThingsToNotice(Catalyst& window) {
    // TODO: Port `TwoThingsToNotice.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void RecapCircleSolution(Catalyst& window) {
    // TODO: Port `RecapCircleSolution.construct()` (base: GraphRectangles)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void ExampleIntegralProblems(Catalyst& window) {
    // TODO: Port `ExampleIntegralProblems.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void MathematicianPonderingAreaUnderDifferentCurves(Catalyst& window) {
    // TODO: Port `MathematicianPonderingAreaUnderDifferentCurves.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void AreaUnderParabola(Catalyst& window) {
    // TODO: Port `AreaUnderParabola.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void WhoCaresAboutArea(Catalyst& window) {
    // TODO: Port `WhoCaresAboutArea.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void PlayWithThisIdea(Catalyst& window) {
    // TODO: Port `PlayWithThisIdea.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void PlayingTowardsDADX(Catalyst& window) {
    // TODO: Port `PlayingTowardsDADX.construct()` (base: AreaUnderParabola)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void NextVideoWrapper(Catalyst& window) {
    // TODO: Port `NextVideoWrapper.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void ProblemSolvingTool(Catalyst& window) {
    // TODO: Port `ProblemSolvingTool.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void FundamentalTheorem(Catalyst& window) {
    // TODO: Port `FundamentalTheorem.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void NextVideos(Catalyst& window) {
    // TODO: Port `NextVideos.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

static void EndScreen(Catalyst& window) {
    // TODO: Port `EndScreen.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/eoc/chapter1.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"Eoc1Thumbnail", &Eoc1Thumbnail},
    {"CircleScene", &CircleScene},
    {"Introduction", &Introduction},
    {"PreviewFrame", &PreviewFrame},
    {"ProductRuleDiagram", &ProductRuleDiagram},
    {"IntroduceCircle", &IntroduceCircle},
    {"ApproximateOneRing", &ApproximateOneRing},
    {"MoveForwardWithApproximation", &MoveForwardWithApproximation},
    {"GraphRectangles", &GraphRectangles},
    {"ThinkLikeAMathematician", &ThinkLikeAMathematician},
    {"TwoThingsToNotice", &TwoThingsToNotice},
    {"RecapCircleSolution", &RecapCircleSolution},
    {"ExampleIntegralProblems", &ExampleIntegralProblems},
    {"MathematicianPonderingAreaUnderDifferentCurves", &MathematicianPonderingAreaUnderDifferentCurves},
    {"AreaUnderParabola", &AreaUnderParabola},
    {"WhoCaresAboutArea", &WhoCaresAboutArea},
    {"PlayWithThisIdea", &PlayWithThisIdea},
    {"PlayingTowardsDADX", &PlayingTowardsDADX},
    {"NextVideoWrapper", &NextVideoWrapper},
    {"ProblemSolvingTool", &ProblemSolvingTool},
    {"FundamentalTheorem", &FundamentalTheorem},
    {"NextVideos", &NextVideos},
    {"EndScreen", &EndScreen},
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

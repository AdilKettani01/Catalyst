// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2016/eola/chapter4.py
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

static void OpeningQuote(Catalyst& window) {
    // TODO: Port `OpeningQuote.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void MatrixToBlank(Catalyst& window) {
    // TODO: Port `MatrixToBlank.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void ExampleTransformation(Catalyst& window) {
    // TODO: Port `ExampleTransformation.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void RecapTime(Catalyst& window) {
    // TODO: Port `RecapTime.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void DeterminedByTwoBasisVectors(Catalyst& window) {
    // TODO: Port `DeterminedByTwoBasisVectors.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void FollowLinearCombination(Catalyst& window) {
    // TODO: Port `FollowLinearCombination.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void RecapOver(Catalyst& window) {
    // TODO: Port `RecapOver.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void TwoSuccessiveTransformations(Catalyst& window) {
    // TODO: Port `TwoSuccessiveTransformations.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void RotationThenShear(Catalyst& window) {
    // TODO: Port `RotationThenShear.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void ExplainWhyItsMatrixMultiplication(Catalyst& window) {
    // TODO: Port `ExplainWhyItsMatrixMultiplication.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void MoreComplicatedExampleVisually(Catalyst& window) {
    // TODO: Port `MoreComplicatedExampleVisually.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void NeverForget(Catalyst& window) {
    // TODO: Port `NeverForget.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void AskAboutCommutativity(Catalyst& window) {
    // TODO: Port `AskAboutCommutativity.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void ShowShear(Catalyst& window) {
    // TODO: Port `ShowShear.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void FirstShearThenRotation(Catalyst& window) {
    // TODO: Port `FirstShearThenRotation.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void NoticeTheLackOfComputations(Catalyst& window) {
    // TODO: Port `NoticeTheLackOfComputations.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void AskAssociativityQuestion(Catalyst& window) {
    // TODO: Port `AskAssociativityQuestion.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void ThreeSuccessiveTransformations(Catalyst& window) {
    // TODO: Port `ThreeSuccessiveTransformations.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void ExplanationTrumpsProof(Catalyst& window) {
    // TODO: Port `ExplanationTrumpsProof.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void GoPlay(Catalyst& window) {
    // TODO: Port `GoPlay.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

static void NextVideo(Catalyst& window) {
    // TODO: Port `NextVideo.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/eola/chapter4.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"OpeningQuote", &OpeningQuote},
    {"MatrixToBlank", &MatrixToBlank},
    {"ExampleTransformation", &ExampleTransformation},
    {"RecapTime", &RecapTime},
    {"DeterminedByTwoBasisVectors", &DeterminedByTwoBasisVectors},
    {"FollowLinearCombination", &FollowLinearCombination},
    {"RecapOver", &RecapOver},
    {"TwoSuccessiveTransformations", &TwoSuccessiveTransformations},
    {"RotationThenShear", &RotationThenShear},
    {"ExplainWhyItsMatrixMultiplication", &ExplainWhyItsMatrixMultiplication},
    {"MoreComplicatedExampleVisually", &MoreComplicatedExampleVisually},
    {"NeverForget", &NeverForget},
    {"AskAboutCommutativity", &AskAboutCommutativity},
    {"ShowShear", &ShowShear},
    {"FirstShearThenRotation", &FirstShearThenRotation},
    {"NoticeTheLackOfComputations", &NoticeTheLackOfComputations},
    {"AskAssociativityQuestion", &AskAssociativityQuestion},
    {"ThreeSuccessiveTransformations", &ThreeSuccessiveTransformations},
    {"ExplanationTrumpsProof", &ExplanationTrumpsProof},
    {"GoPlay", &GoPlay},
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

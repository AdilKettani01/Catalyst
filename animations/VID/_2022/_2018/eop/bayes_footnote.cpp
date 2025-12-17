// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2018/eop/bayes_footnote.py
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

static void Introduction(Catalyst& window) {
    // TODO: Port `Introduction.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void OneInOneThousandHaveDisease(Catalyst& window) {
    // TODO: Port `OneInOneThousandHaveDisease.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void TestScene(Catalyst& window) {
    // TODO: Port `TestScene.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void TestDiseaseCase(Catalyst& window) {
    // TODO: Port `TestDiseaseCase.construct()` (base: TestScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void TestNonDiseaseCase(Catalyst& window) {
    // TODO: Port `TestNonDiseaseCase.construct()` (base: TestScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void ReceivePositiveResults(Catalyst& window) {
    // TODO: Port `ReceivePositiveResults.construct()` (base: TestScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void AskAboutRephrasingQuestion(Catalyst& window) {
    // TODO: Port `AskAboutRephrasingQuestion.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void RephraseQuestion(Catalyst& window) {
    // TODO: Port `RephraseQuestion.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void TryUnitSquareVisual(Catalyst& window) {
    // TODO: Port `TryUnitSquareVisual.construct()` (base: SampleSpaceScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void ShowRestrictedSpace(Catalyst& window) {
    // TODO: Port `ShowRestrictedSpace.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void DepressingForMedicalTestDesigners(Catalyst& window) {
    // TODO: Port `DepressingForMedicalTestDesigners.construct()` (base: TestScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void HowMuchCanYouChangeThisPrior(Catalyst& window) {
    // TODO: Port `HowMuchCanYouChangeThisPrior.construct()` (base: ShowRestrictedSpace)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void ShowTheFormula(Catalyst& window) {
    // TODO: Port `ShowTheFormula.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void SourceOfConfusion(Catalyst& window) {
    // TODO: Port `SourceOfConfusion.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void StatisticsVsEmpathy(Catalyst& window) {
    // TODO: Port `StatisticsVsEmpathy.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void LessMedicalExample(Catalyst& window) {
    // TODO: Port `LessMedicalExample.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void PlaneCrashProbability(Catalyst& window) {
    // TODO: Port `PlaneCrashProbability.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void CompareNumbersInBothExamples(Catalyst& window) {
    // TODO: Port `CompareNumbersInBothExamples.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void NonchalantReactionToPositiveTest(Catalyst& window) {
    // TODO: Port `NonchalantReactionToPositiveTest.construct()` (base: TestScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void AlwaysPictureTheSpaceOfPossibilities(Catalyst& window) {
    // TODO: Port `AlwaysPictureTheSpaceOfPossibilities.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

static void Thumbnail(Catalyst& window) {
    // TODO: Port `Thumbnail.construct()` (base: Scene)
    // From: animations/VID/_2022/_2018/eop/bayes_footnote.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"Introduction", &Introduction},
    {"OneInOneThousandHaveDisease", &OneInOneThousandHaveDisease},
    {"TestScene", &TestScene},
    {"TestDiseaseCase", &TestDiseaseCase},
    {"TestNonDiseaseCase", &TestNonDiseaseCase},
    {"ReceivePositiveResults", &ReceivePositiveResults},
    {"AskAboutRephrasingQuestion", &AskAboutRephrasingQuestion},
    {"RephraseQuestion", &RephraseQuestion},
    {"TryUnitSquareVisual", &TryUnitSquareVisual},
    {"ShowRestrictedSpace", &ShowRestrictedSpace},
    {"DepressingForMedicalTestDesigners", &DepressingForMedicalTestDesigners},
    {"HowMuchCanYouChangeThisPrior", &HowMuchCanYouChangeThisPrior},
    {"ShowTheFormula", &ShowTheFormula},
    {"SourceOfConfusion", &SourceOfConfusion},
    {"StatisticsVsEmpathy", &StatisticsVsEmpathy},
    {"LessMedicalExample", &LessMedicalExample},
    {"PlaneCrashProbability", &PlaneCrashProbability},
    {"CompareNumbersInBothExamples", &CompareNumbersInBothExamples},
    {"NonchalantReactionToPositiveTest", &NonchalantReactionToPositiveTest},
    {"AlwaysPictureTheSpaceOfPossibilities", &AlwaysPictureTheSpaceOfPossibilities},
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

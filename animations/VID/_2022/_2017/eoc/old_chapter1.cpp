// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2017/eoc/old_chapter1.py
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

static void CircleScene(Catalyst& window) {
    // TODO: Port `CircleScene.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void PatronsOnly(Catalyst& window) {
    // TODO: Port `PatronsOnly.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void Introduction(Catalyst& window) {
    // TODO: Port `Introduction.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void IntroduceCircle(Catalyst& window) {
    // TODO: Port `IntroduceCircle.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void HeartOfCalculus(Catalyst& window) {
    // TODO: Port `HeartOfCalculus.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void PragmatismToArt(Catalyst& window) {
    // TODO: Port `PragmatismToArt.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void IntroduceTinyChangeInArea(Catalyst& window) {
    // TODO: Port `IntroduceTinyChangeInArea.construct()` (base: CircleScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void CleanUpABit(Catalyst& window) {
    // TODO: Port `CleanUpABit.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void BuildToDADR(Catalyst& window) {
    // TODO: Port `BuildToDADR.construct()` (base: CircleScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void DerivativeAsTangentLine(Catalyst& window) {
    // TODO: Port `DerivativeAsTangentLine.construct()` (base: ZoomedScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void SimpleConfusedPi(Catalyst& window) {
    // TODO: Port `SimpleConfusedPi.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void TangentLinesAreNotEverything(Catalyst& window) {
    // TODO: Port `TangentLinesAreNotEverything.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void OnToIntegrals(Catalyst& window) {
    // TODO: Port `OnToIntegrals.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void IntroduceConcentricRings(Catalyst& window) {
    // TODO: Port `IntroduceConcentricRings.construct()` (base: CircleScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void AskAboutGeneralCircles(Catalyst& window) {
    // TODO: Port `AskAboutGeneralCircles.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void GraphIntegral(Catalyst& window) {
    // TODO: Port `GraphIntegral.construct()` (base: GraphScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void MoreOnThisLater(Catalyst& window) {
    // TODO: Port `MoreOnThisLater.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void FundamentalTheorem(Catalyst& window) {
    // TODO: Port `FundamentalTheorem.construct()` (base: CircleScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void NameTheFundamentalTheorem(Catalyst& window) {
    // TODO: Port `NameTheFundamentalTheorem.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void CalculusInANutshell(Catalyst& window) {
    // TODO: Port `CalculusInANutshell.construct()` (base: CircleScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

static void Thumbnail(Catalyst& window) {
    // TODO: Port `Thumbnail.construct()` (base: CircleScene)
    // From: animations/VID/_2022/_2017/eoc/old_chapter1.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"CircleScene", &CircleScene},
    {"PatronsOnly", &PatronsOnly},
    {"Introduction", &Introduction},
    {"IntroduceCircle", &IntroduceCircle},
    {"HeartOfCalculus", &HeartOfCalculus},
    {"PragmatismToArt", &PragmatismToArt},
    {"IntroduceTinyChangeInArea", &IntroduceTinyChangeInArea},
    {"CleanUpABit", &CleanUpABit},
    {"BuildToDADR", &BuildToDADR},
    {"DerivativeAsTangentLine", &DerivativeAsTangentLine},
    {"SimpleConfusedPi", &SimpleConfusedPi},
    {"TangentLinesAreNotEverything", &TangentLinesAreNotEverything},
    {"OnToIntegrals", &OnToIntegrals},
    {"IntroduceConcentricRings", &IntroduceConcentricRings},
    {"AskAboutGeneralCircles", &AskAboutGeneralCircles},
    {"GraphIntegral", &GraphIntegral},
    {"MoreOnThisLater", &MoreOnThisLater},
    {"FundamentalTheorem", &FundamentalTheorem},
    {"NameTheFundamentalTheorem", &NameTheFundamentalTheorem},
    {"CalculusInANutshell", &CalculusInANutshell},
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

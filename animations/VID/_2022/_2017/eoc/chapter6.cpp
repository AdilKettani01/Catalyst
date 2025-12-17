// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2017/eoc/chapter6.py
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

static void ThisWasConfusing(Catalyst& window) {
    // TODO: Port `ThisWasConfusing.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void SlopeOfCircleExample(Catalyst& window) {
    // TODO: Port `SlopeOfCircleExample.construct()` (base: ZoomedScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void NameImplicitDifferentation(Catalyst& window) {
    // TODO: Port `NameImplicitDifferentation.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void RelatedRatesExample(Catalyst& window) {
    // TODO: Port `RelatedRatesExample.construct()` (base: ThreeDScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void CompareLadderAndCircle(Catalyst& window) {
    // TODO: Port `CompareLadderAndCircle.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void TryOtherExamples(Catalyst& window) {
    // TODO: Port `TryOtherExamples.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void AlternateExample(Catalyst& window) {
    // TODO: Port `AlternateExample.construct()` (base: ZoomedScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void AskAboutNaturalLog(Catalyst& window) {
    // TODO: Port `AskAboutNaturalLog.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void DerivativeOfNaturalLog(Catalyst& window) {
    // TODO: Port `DerivativeOfNaturalLog.construct()` (base: ZoomedScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

static void FinalWords(Catalyst& window) {
    // TODO: Port `FinalWords.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/eoc/chapter6.py
    // NOTE: Source appears to use 3D constructs; you may need `window.set3DMode(true)` and camera controls.
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"ThisWasConfusing", &ThisWasConfusing},
    {"SlopeOfCircleExample", &SlopeOfCircleExample},
    {"NameImplicitDifferentation", &NameImplicitDifferentation},
    {"RelatedRatesExample", &RelatedRatesExample},
    {"CompareLadderAndCircle", &CompareLadderAndCircle},
    {"TryOtherExamples", &TryOtherExamples},
    {"AlternateExample", &AlternateExample},
    {"AskAboutNaturalLog", &AskAboutNaturalLog},
    {"DerivativeOfNaturalLog", &DerivativeOfNaturalLog},
    {"FinalWords", &FinalWords},
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

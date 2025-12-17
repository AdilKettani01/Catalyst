// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2017/tattoo.py
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

static void TrigRepresentationsScene(Catalyst& window) {
    // TODO: Port `TrigRepresentationsScene.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

static void Introduce(Catalyst& window) {
    // TODO: Port `Introduce.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

static void ReactionsToTattoo(Catalyst& window) {
    // TODO: Port `ReactionsToTattoo.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

static void IntroduceCSC(Catalyst& window) {
    // TODO: Port `IntroduceCSC.construct()` (base: TrigRepresentationsScene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

static void TeachObscureTrigFunctions(Catalyst& window) {
    // TODO: Port `TeachObscureTrigFunctions.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

static void CanYouExplainTheTattoo(Catalyst& window) {
    // TODO: Port `CanYouExplainTheTattoo.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

static void ExplainTrigFunctionDistances(Catalyst& window) {
    // TODO: Port `ExplainTrigFunctionDistances.construct()` (base: TrigRepresentationsScene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

static void RenameAllInTermsOfSine(Catalyst& window) {
    // TODO: Port `RenameAllInTermsOfSine.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

static void MisMatchOfCoPrefix(Catalyst& window) {
    // TODO: Port `MisMatchOfCoPrefix.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

static void Credit(Catalyst& window) {
    // TODO: Port `Credit.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/tattoo.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"TrigRepresentationsScene", &TrigRepresentationsScene},
    {"Introduce", &Introduce},
    {"ReactionsToTattoo", &ReactionsToTattoo},
    {"IntroduceCSC", &IntroduceCSC},
    {"TeachObscureTrigFunctions", &TeachObscureTrigFunctions},
    {"CanYouExplainTheTattoo", &CanYouExplainTheTattoo},
    {"ExplainTrigFunctionDistances", &ExplainTrigFunctionDistances},
    {"RenameAllInTermsOfSine", &RenameAllInTermsOfSine},
    {"MisMatchOfCoPrefix", &MisMatchOfCoPrefix},
    {"Credit", &Credit},
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

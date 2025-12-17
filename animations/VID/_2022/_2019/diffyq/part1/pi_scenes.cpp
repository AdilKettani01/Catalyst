// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2019/diffyq/part1/pi_scenes.py
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

static void SomeOfYouWatching(Catalyst& window) {
    // TODO: Port `SomeOfYouWatching.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part1/pi_scenes.py
    (void)window;
}

static void FormulasAreLies(Catalyst& window) {
    // TODO: Port `FormulasAreLies.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2019/diffyq/part1/pi_scenes.py
    (void)window;
}

static void SoWhatIsThetaThen(Catalyst& window) {
    // TODO: Port `SoWhatIsThetaThen.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part1/pi_scenes.py
    (void)window;
}

static void ProveTeacherWrong(Catalyst& window) {
    // TODO: Port `ProveTeacherWrong.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part1/pi_scenes.py
    (void)window;
}

static void PhysicistPhaseSpace(Catalyst& window) {
    // TODO: Port `PhysicistPhaseSpace.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2019/diffyq/part1/pi_scenes.py
    (void)window;
}

static void AskAboutActuallySolving(Catalyst& window) {
    // TODO: Port `AskAboutActuallySolving.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part1/pi_scenes.py
    (void)window;
}

static void HungerForExactness(Catalyst& window) {
    // TODO: Port `HungerForExactness.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part1/pi_scenes.py
    (void)window;
}

static void ItGetsWorse(Catalyst& window) {
    // TODO: Port `ItGetsWorse.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2019/diffyq/part1/pi_scenes.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"SomeOfYouWatching", &SomeOfYouWatching},
    {"FormulasAreLies", &FormulasAreLies},
    {"SoWhatIsThetaThen", &SoWhatIsThetaThen},
    {"ProveTeacherWrong", &ProveTeacherWrong},
    {"PhysicistPhaseSpace", &PhysicistPhaseSpace},
    {"AskAboutActuallySolving", &AskAboutActuallySolving},
    {"HungerForExactness", &HungerForExactness},
    {"ItGetsWorse", &ItGetsWorse},
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

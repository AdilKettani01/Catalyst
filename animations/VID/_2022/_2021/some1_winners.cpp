// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2021/some1_winners.py
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
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

static void ProsConsOfContext(Catalyst& window) {
    // TODO: Port `ProsConsOfContext.construct()` (base: Scene)
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

static void FiltrationProcess(Catalyst& window) {
    // TODO: Port `FiltrationProcess.construct()` (base: Scene)
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

static void AllVideosOrdered(Catalyst& window) {
    // TODO: Port `AllVideosOrdered.construct()` (base: Scene)
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

static void RevealingTiles(Catalyst& window) {
    // TODO: Port `RevealingTiles.construct()` (base: Scene)
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

static void AlmostTooGood(Catalyst& window) {
    // TODO: Port `AlmostTooGood.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

static void ViewRect(Catalyst& window) {
    // TODO: Port `ViewRect.construct()` (base: Scene)
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

static void SureSure(Catalyst& window) {
    // TODO: Port `SureSure.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

static void Narrative(Catalyst& window) {
    // TODO: Port `Narrative.construct()` (base: Scene)
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

static void Traction(Catalyst& window) {
    // TODO: Port `Traction.construct()` (base: Scene)
    // From: animations/VID/_2022/_2021/some1_winners.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"Introduction", &Introduction},
    {"ProsConsOfContext", &ProsConsOfContext},
    {"FiltrationProcess", &FiltrationProcess},
    {"AllVideosOrdered", &AllVideosOrdered},
    {"RevealingTiles", &RevealingTiles},
    {"AlmostTooGood", &AlmostTooGood},
    {"ViewRect", &ViewRect},
    {"SureSure", &SureSure},
    {"Narrative", &Narrative},
    {"Traction", &Traction},
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

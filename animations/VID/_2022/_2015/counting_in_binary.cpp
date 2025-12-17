// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2015/counting_in_binary.py
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

static void CountTo1023(Catalyst& window) {
    // TODO: Port `CountTo1023.construct()` (base: Scene)
    // From: animations/VID/_2022/_2015/counting_in_binary.py
    (void)window;
}

static void Introduction(Catalyst& window) {
    // TODO: Port `Introduction.construct()` (base: Scene)
    // From: animations/VID/_2022/_2015/counting_in_binary.py
    (void)window;
}

static void ShowReadingRule(Catalyst& window) {
    // TODO: Port `ShowReadingRule.construct()` (base: Scene)
    // From: animations/VID/_2022/_2015/counting_in_binary.py
    (void)window;
}

static void ShowIncrementRule(Catalyst& window) {
    // TODO: Port `ShowIncrementRule.construct()` (base: Scene)
    // From: animations/VID/_2022/_2015/counting_in_binary.py
    (void)window;
}

static void MindFindsShortcuts(Catalyst& window) {
    // TODO: Port `MindFindsShortcuts.construct()` (base: Scene)
    // From: animations/VID/_2022/_2015/counting_in_binary.py
    (void)window;
}

static void FinishCountingExampleSentence(Catalyst& window) {
    // TODO: Port `FinishCountingExampleSentence.construct()` (base: Scene)
    // From: animations/VID/_2022/_2015/counting_in_binary.py
    (void)window;
}

static void Question(Catalyst& window) {
    // TODO: Port `Question.construct()` (base: Scene)
    // From: animations/VID/_2022/_2015/counting_in_binary.py
    (void)window;
}

static void TwoHandStatement(Catalyst& window) {
    // TODO: Port `TwoHandStatement.construct()` (base: Scene)
    // From: animations/VID/_2022/_2015/counting_in_binary.py
    (void)window;
}

static void WithToes(Catalyst& window) {
    // TODO: Port `WithToes.construct()` (base: Scene)
    // From: animations/VID/_2022/_2015/counting_in_binary.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"CountTo1023", &CountTo1023},
    {"Introduction", &Introduction},
    {"ShowReadingRule", &ShowReadingRule},
    {"ShowIncrementRule", &ShowIncrementRule},
    {"MindFindsShortcuts", &MindFindsShortcuts},
    {"FinishCountingExampleSentence", &FinishCountingExampleSentence},
    {"Question", &Question},
    {"TwoHandStatement", &TwoHandStatement},
    {"WithToes", &WithToes},
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

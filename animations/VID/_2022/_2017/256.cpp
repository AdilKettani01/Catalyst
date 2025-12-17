// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2017/256.py
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

static void LastVideo(Catalyst& window) {
    // TODO: Port `LastVideo.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/256.py
    (void)window;
}

static void BreakUp2To256(Catalyst& window) {
    // TODO: Port `BreakUp2To256.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/256.py
    (void)window;
}

static void ShowTwoTo32(Catalyst& window) {
    // TODO: Port `ShowTwoTo32.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/256.py
    (void)window;
}

static void MainBreakdown(Catalyst& window) {
    // TODO: Port `MainBreakdown.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/256.py
    (void)window;
}

static void WriteTWoTo160(Catalyst& window) {
    // TODO: Port `WriteTWoTo160.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/256.py
    (void)window;
}

static void StateOfBitcoin(Catalyst& window) {
    // TODO: Port `StateOfBitcoin.construct()` (base: TeacherStudentsScene)
    // From: animations/VID/_2022/_2017/256.py
    (void)window;
}

static void QAndA(Catalyst& window) {
    // TODO: Port `QAndA.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2017/256.py
    (void)window;
}

static void Thumbnail(Catalyst& window) {
    // TODO: Port `Thumbnail.construct()` (base: Scene)
    // From: animations/VID/_2022/_2017/256.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"LastVideo", &LastVideo},
    {"BreakUp2To256", &BreakUp2To256},
    {"ShowTwoTo32", &ShowTwoTo32},
    {"MainBreakdown", &MainBreakdown},
    {"WriteTWoTo160", &WriteTWoTo160},
    {"StateOfBitcoin", &StateOfBitcoin},
    {"QAndA", &QAndA},
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

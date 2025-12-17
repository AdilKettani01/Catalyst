// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2016/eola/thumbnails.py
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

static void Chapter0(Catalyst& window) {
    // TODO: Port `Chapter0.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/thumbnails.py
    (void)window;
}

static void Chapter1(Catalyst& window) {
    // TODO: Port `Chapter1.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/eola/thumbnails.py
    (void)window;
}

static void Chapter2(Catalyst& window) {
    // TODO: Port `Chapter2.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/thumbnails.py
    (void)window;
}

static void Chapter5(Catalyst& window) {
    // TODO: Port `Chapter5.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/thumbnails.py
    (void)window;
}

static void Chapter9(Catalyst& window) {
    // TODO: Port `Chapter9.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/eola/thumbnails.py
    (void)window;
}

static void Chapter10(Catalyst& window) {
    // TODO: Port `Chapter10.construct()` (base: LinearTransformationScene)
    // From: animations/VID/_2022/_2016/eola/thumbnails.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"Chapter0", &Chapter0},
    {"Chapter1", &Chapter1},
    {"Chapter2", &Chapter2},
    {"Chapter5", &Chapter5},
    {"Chapter9", &Chapter9},
    {"Chapter10", &Chapter10},
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

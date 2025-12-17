// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2016/brachistochrone/light.py
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

static void PhotonScene(Catalyst& window) {
    // TODO: Port `PhotonScene.construct()` (base: Scene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void SimplePhoton(Catalyst& window) {
    // TODO: Port `SimplePhoton.construct()` (base: PhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void MultipathPhotonScene(Catalyst& window) {
    // TODO: Port `MultipathPhotonScene.construct()` (base: PhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void PhotonThroughLens(Catalyst& window) {
    // TODO: Port `PhotonThroughLens.construct()` (base: MultipathPhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void PhotonOffMirror(Catalyst& window) {
    // TODO: Port `PhotonOffMirror.construct()` (base: MultipathPhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void PhotonsInWater(Catalyst& window) {
    // TODO: Port `PhotonsInWater.construct()` (base: MultipathPhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void ShowMultiplePathsScene(Catalyst& window) {
    // TODO: Port `ShowMultiplePathsScene.construct()` (base: PhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void ShowMultiplePathsThroughLens(Catalyst& window) {
    // TODO: Port `ShowMultiplePathsThroughLens.construct()` (base: ShowMultiplePathsScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void ShowMultiplePathsOffMirror(Catalyst& window) {
    // TODO: Port `ShowMultiplePathsOffMirror.construct()` (base: ShowMultiplePathsScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void ShowMultiplePathsInWater(Catalyst& window) {
    // TODO: Port `ShowMultiplePathsInWater.construct()` (base: ShowMultiplePathsScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void StraightLinesFastestInConstantMedium(Catalyst& window) {
    // TODO: Port `StraightLinesFastestInConstantMedium.construct()` (base: PhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void PhtonBendsInWater(Catalyst& window) {
    // TODO: Port `PhtonBendsInWater.construct()` (base: PhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void WhatGovernsTheSpeedOfLight(Catalyst& window) {
    // TODO: Port `WhatGovernsTheSpeedOfLight.construct()` (base: PhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void WhichPathWouldLightTake(Catalyst& window) {
    // TODO: Port `WhichPathWouldLightTake.construct()` (base: PhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

static void StateSnellsLaw(Catalyst& window) {
    // TODO: Port `StateSnellsLaw.construct()` (base: PhotonScene)
    // From: animations/VID/_2022/_2016/brachistochrone/light.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"PhotonScene", &PhotonScene},
    {"SimplePhoton", &SimplePhoton},
    {"MultipathPhotonScene", &MultipathPhotonScene},
    {"PhotonThroughLens", &PhotonThroughLens},
    {"PhotonOffMirror", &PhotonOffMirror},
    {"PhotonsInWater", &PhotonsInWater},
    {"ShowMultiplePathsScene", &ShowMultiplePathsScene},
    {"ShowMultiplePathsThroughLens", &ShowMultiplePathsThroughLens},
    {"ShowMultiplePathsOffMirror", &ShowMultiplePathsOffMirror},
    {"ShowMultiplePathsInWater", &ShowMultiplePathsInWater},
    {"StraightLinesFastestInConstantMedium", &StraightLinesFastestInConstantMedium},
    {"PhtonBendsInWater", &PhtonBendsInWater},
    {"WhatGovernsTheSpeedOfLight", &WhatGovernsTheSpeedOfLight},
    {"WhichPathWouldLightTake", &WhichPathWouldLightTake},
    {"StateSnellsLaw", &StateSnellsLaw},
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

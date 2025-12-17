// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2025/cosmic_distance/planets.py
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

static void PerspectivesOnEarth(Catalyst& window) {
    // TODO: Port `PerspectivesOnEarth.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void SphericalEarthVsFlat(Catalyst& window) {
    // TODO: Port `SphericalEarthVsFlat.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void SizeOfEarthRenewed(Catalyst& window) {
    // TODO: Port `SizeOfEarthRenewed.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void AlBiruniEarthMeasurement(Catalyst& window) {
    // TODO: Port `AlBiruniEarthMeasurement.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void LuarEclipse(Catalyst& window) {
    // TODO: Port `LuarEclipse.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void PenumbraAndUmbra(Catalyst& window) {
    // TODO: Port `PenumbraAndUmbra.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void LineOfSight(Catalyst& window) {
    // TODO: Port `LineOfSight.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void DistanceToSun(Catalyst& window) {
    // TODO: Port `DistanceToSun.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void PhasesOfTheMoon(Catalyst& window) {
    // TODO: Port `PhasesOfTheMoon.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void HowManyEarthsInsideSun(Catalyst& window) {
    // TODO: Port `HowManyEarthsInsideSun.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void EarthAroundSun(Catalyst& window) {
    // TODO: Port `EarthAroundSun.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void NearestPlanets(Catalyst& window) {
    // TODO: Port `NearestPlanets.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void KeplersMethod(Catalyst& window) {
    // TODO: Port `KeplersMethod.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void LightFromEarthToMoon(Catalyst& window) {
    // TODO: Port `LightFromEarthToMoon.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

static void EarthAndVenus(Catalyst& window) {
    // TODO: Port `EarthAndVenus.construct()` (base: InteractiveScene)
    // From: animations/VID/_2025/cosmic_distance/planets.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"PerspectivesOnEarth", &PerspectivesOnEarth},
    {"SphericalEarthVsFlat", &SphericalEarthVsFlat},
    {"SizeOfEarthRenewed", &SizeOfEarthRenewed},
    {"AlBiruniEarthMeasurement", &AlBiruniEarthMeasurement},
    {"LuarEclipse", &LuarEclipse},
    {"PenumbraAndUmbra", &PenumbraAndUmbra},
    {"LineOfSight", &LineOfSight},
    {"DistanceToSun", &DistanceToSun},
    {"PhasesOfTheMoon", &PhasesOfTheMoon},
    {"HowManyEarthsInsideSun", &HowManyEarthsInsideSun},
    {"EarthAroundSun", &EarthAroundSun},
    {"NearestPlanets", &NearestPlanets},
    {"KeplersMethod", &KeplersMethod},
    {"LightFromEarthToMoon", &LightFromEarthToMoon},
    {"EarthAndVenus", &EarthAndVenus},
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

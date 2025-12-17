// AUTO-GENERATED Catalyst translation scaffold.
// Source: animations/VID/_2022/_2019/diffyq/part3/staging.py
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

static void FourierNameIntro(Catalyst& window) {
    // TODO: Port `FourierNameIntro.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/staging.py
    (void)window;
}

static void ManyCousinsOfFourierThings(Catalyst& window) {
    // TODO: Port `ManyCousinsOfFourierThings.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/staging.py
    (void)window;
}

static void FourierSeriesIllustraiton(Catalyst& window) {
    // TODO: Port `FourierSeriesIllustraiton.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/staging.py
    (void)window;
}

static void ExponentialDecay(Catalyst& window) {
    // TODO: Port `ExponentialDecay.construct()` (base: PiCreatureScene)
    // From: animations/VID/_2022/_2019/diffyq/part3/staging.py
    (void)window;
}

static void InvestmentGrowth(Catalyst& window) {
    // TODO: Port `InvestmentGrowth.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/staging.py
    (void)window;
}

static void CarbonDecayingInMammoth(Catalyst& window) {
    // TODO: Port `CarbonDecayingInMammoth.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/staging.py
    (void)window;
}

static void BoundaryConditionInterlude(Catalyst& window) {
    // TODO: Port `BoundaryConditionInterlude.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/staging.py
    (void)window;
}

static void GiantCross(Catalyst& window) {
    // TODO: Port `GiantCross.construct()` (base: Scene)
    // From: animations/VID/_2022/_2019/diffyq/part3/staging.py
    (void)window;
}

struct SceneEntry {
    const char* name;
    void (*fn)(Catalyst&);
};

static constexpr SceneEntry kScenes[] = {
    {"FourierNameIntro", &FourierNameIntro},
    {"ManyCousinsOfFourierThings", &ManyCousinsOfFourierThings},
    {"FourierSeriesIllustraiton", &FourierSeriesIllustraiton},
    {"ExponentialDecay", &ExponentialDecay},
    {"InvestmentGrowth", &InvestmentGrowth},
    {"CarbonDecayingInMammoth", &CarbonDecayingInMammoth},
    {"BoundaryConditionInterlude", &BoundaryConditionInterlude},
    {"GiantCross", &GiantCross},
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

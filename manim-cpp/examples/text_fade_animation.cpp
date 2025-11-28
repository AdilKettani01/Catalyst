/**
 * @file text_fade_animation.cpp
 * @brief Text fade animation example
 *
 * Demonstrates:
 * - GPU-accelerated SDF text rendering
 * - FadeIn/FadeOut animations
 * - Sequential animation timeline
 *
 * Build and run:
 *   ./bin/text_fade_animation
 *
 * Output: text_animation.mp4
 */

#include "manim/mobject/text/text.hpp"
#include "manim/animation/fading.hpp"
#include "manim/scene/scene.h"
#include <memory>
#include <iostream>

class TextFadeScene : public manim::Scene {
public:
    void construct() override {
        // Create text objects with 72pt font size
        auto text1 = std::make_shared<manim::Text>("TEXT1", 72.0f);
        auto text2 = std::make_shared<manim::Text>("TEXT2", 72.0f);
        auto text3 = std::make_shared<manim::Text>("TEXT3", 72.0f);

        // Set white color for text (optional, default may be white)
        text1->set_color(manim::math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});
        text2->set_color(manim::math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});
        text3->set_color(manim::math::Vec4{1.0f, 1.0f, 1.0f, 1.0f});

        // TEXT1: Fade in, wait 4 seconds, fade out
        play(std::make_shared<manim::FadeIn>(text1, 1.0f));
        wait(4.0);
        play(std::make_shared<manim::FadeOut>(text1, 1.0f));

        // TEXT2: Fade in, wait 4 seconds, fade out
        play(std::make_shared<manim::FadeIn>(text2, 1.0f));
        wait(4.0);
        play(std::make_shared<manim::FadeOut>(text2, 1.0f));

        // TEXT3: Fade in, wait 4 seconds, fade out
        play(std::make_shared<manim::FadeIn>(text3, 1.0f));
        wait(4.0);
        play(std::make_shared<manim::FadeOut>(text3, 1.0f));
    }
};

int main() {
    std::cout << "Creating text fade animation...\n";

    TextFadeScene scene;
    scene.setName("TextFadeAnimation");
    scene.render();

    std::cout << "Animation complete!\n";
    return 0;
}

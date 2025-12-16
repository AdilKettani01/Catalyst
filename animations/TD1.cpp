// Comprehensive Feature Demo - All Catalyst features demonstrated
#include "../catalyst.h"
#include <vector>
#include <cmath>

int main() {
    Catalyst window(1280 , 720);

    // ============================================================================
    // SECTION 1: TEXT & TYPOGRAPHY
    // ============================================================================

    auto sectionTitle = window.setText("Section 1: Text & Typography");
    sectionTitle.setPosition(Position::TLEFT);
    sectionTitle.setSize(36);
    sectionTitle.setColor("#FFFFFF");
    sectionTitle.show(5.5f);

    // 1.1 Basic text rendering
     window.wait(5.0f);
     auto cmdLabel = window.setText("setText(\"Hello Catalyst\")");
     cmdLabel.setPosition(Position::TRIGHT);cmdLabel.setSize(24);
     cmdLabel.setColor("#00FF00");
     cmdLabel.show(5.0f);
     auto demo = window.setText("Hello Catalyst");
    demo.setSize(72);
    demo.setPosition(Position::CENTER);
    demo.setColor("#FFFFFF");
    demo.show(5.0f);

      window.wait(5.0f);
      demo.hide(0.5f);
      cmdLabel.hide(5.0f);
    //
    // // 1.2 Font size control
    // window.wait(5.0f);
    // cmdLabel = window.setText("setSize(24) -> setSize(48) -> setSize(72)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(5.0f);
    //
    // auto sizeDemo = window.setText("Font Size");
    // sizeDemo.setSize(24);
    // sizeDemo.setPosition(Position::CENTER);
    // sizeDemo.setColor("#FFFFFF");
    // sizeDemo.show(5.0f);
    //
    // window.wait(5.0f);
    // sizeDemo.hide(0.3f);
    // window.wait(5.0f);
    // sizeDemo = window.setText("Font Size");
    // sizeDemo.setSize(48);
    // sizeDemo.setPosition(Position::CENTER);
    // sizeDemo.setColor("#FFFFFF");
    // sizeDemo.show(5.0f);
    //
    // window.wait(5.0f);
    // sizeDemo.hide(0.3f);
    // window.wait(0.3f);
    // sizeDemo = window.setText("Font Size");
    // sizeDemo.setSize(72);
    // sizeDemo.setPosition(Position::CENTER);
    // sizeDemo.setColor("#FFFFFF");
    // sizeDemo.show(0.3f);
    //
    // window.wait(5.0f);
    // sizeDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 1.3 Text positioning (pixel)
    // window.wait(0.5f);
    // cmdLabel = window.setText("setPosition(100, 200)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto posDemo = window.setText("Pixel Position (100, 200)");
    // posDemo.setSize(36);
    // posDemo.setPosition(100, 200);
    // posDemo.setColor("#FF5733");
    // posDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // posDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 1.4 Anchor positions
    // window.wait(0.5f);
    // cmdLabel = window.setText("setPosition(Position::ANCHOR)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto anchorTL = window.setText("TLEFT");
    // anchorTL.setSize(24);
    // anchorTL.setPosition(Position::TLEFT);
    // anchorTL.setColor("#FF5733");
    // anchorTL.show(0.3f);
    //
    // auto anchorT = window.setText("TOP");
    // anchorT.setSize(24);
    // anchorT.setPosition(Position::TOP);
    // anchorT.setColor("#33FF57");
    // anchorT.show(0.3f);
    //
    // auto anchorTR = window.setText("TRIGHT");
    // anchorTR.setSize(24);
    // anchorTR.setPosition(Position::TRIGHT);
    // anchorTR.setColor("#5733FF");
    // anchorTR.show(0.3f);
    //
    // auto anchorL = window.setText("LEFT");
    // anchorL.setSize(24);
    // anchorL.setPosition(Position::LEFT);
    // anchorL.setColor("#FFFF00");
    // anchorL.show(0.3f);
    //
    // auto anchorC = window.setText("CENTER");
    // anchorC.setSize(24);
    // anchorC.setPosition(Position::CENTER);
    // anchorC.setColor("#00FFFF");
    // anchorC.show(0.3f);
    //
    // auto anchorR = window.setText("RIGHT");
    // anchorR.setSize(24);
    // anchorR.setPosition(Position::RIGHT);
    // anchorR.setColor("#FF00FF");
    // anchorR.show(0.3f);
    //
    // auto anchorBL = window.setText("BLEFT");
    // anchorBL.setSize(24);
    // anchorBL.setPosition(Position::BLEFT);
    // anchorBL.setColor("#FF8800");
    // anchorBL.show(0.3f);
    //
    // auto anchorB = window.setText("BOTTOM");
    // anchorB.setSize(24);
    // anchorB.setPosition(Position::BOTTOM);
    // anchorB.setColor("#88FF00");
    // anchorB.show(0.3f);
    //
    // auto anchorBR = window.setText("BRIGHT");
    // anchorBR.setSize(24);
    // anchorBR.setPosition(Position::BRIGHT);
    // anchorBR.setColor("#0088FF");
    // anchorBR.show(0.3f);
    //
    // window.wait(5.0f);
    // anchorTL.hide(0.3f);
    // anchorT.hide(0.3f);
    // anchorTR.hide(0.3f);
    // anchorL.hide(0.3f);
    // anchorC.hide(0.3f);
    // anchorR.hide(0.3f);
    // anchorBL.hide(0.3f);
    // anchorB.hide(0.3f);
    // anchorBR.hide(0.3f);
    // cmdLabel.hide(0.3f);
    //
    // // 1.5 Text color (hex)
    // window.wait(0.5f);
    // cmdLabel = window.setText("setColor(\"#FF5733\")");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto colorHex = window.setText("Hex Color #FF5733");
    // colorHex.setSize(48);
    // colorHex.setPosition(Position::CENTER);
    // colorHex.setColor("#FF5733");
    // colorHex.show(0.5f);
    //
    // window.wait(5.0f);
    // colorHex.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 1.6 Text color (RGB)
    // window.wait(0.5f);
    // cmdLabel = window.setText("setColor(100, 200, 255)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto colorRGB = window.setText("RGB Color (100, 200, 255)");
    // colorRGB.setSize(48);
    // colorRGB.setPosition(Position::CENTER);
    // colorRGB.setColor(100, 200, 255);
    // colorRGB.show(0.5f);
    //
    // window.wait(5.0f);
    // colorRGB.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 1.7 Text color (HSL)
    // window.wait(0.5f);
    // cmdLabel = window.setText("setColor(280.0f, 0.8f, 0.6f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto colorHSL = window.setText("HSL Color (280, 0.8, 0.6)");
    // colorHSL.setSize(48);
    // colorHSL.setPosition(Position::CENTER);
    // colorHSL.setColor(280.0f, 0.8f, 0.6f);
    // colorHSL.show(0.5f);
    //
    // window.wait(5.0f);
    // colorHSL.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 1.8 LaTeX/Math
    // window.wait(0.5f);
    // cmdLabel = window.setText("setMath(\"E = mc^2\")");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto mathDemo = window.setMath("E = mc^2");
    // mathDemo.setSize(72);
    // mathDemo.setPosition(Position::CENTER);
    // mathDemo.setColor("#FFFFFF");
    // mathDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // mathDemo.hide(0.3f);
    // cmdLabel.hide(0.3f);
    //
    // window.wait(0.5f);
    // cmdLabel = window.setText("setMath(\"\\\\int_0^\\\\infty e^{-x^2} dx\")");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto mathDemo2 = window.setMath("\\int_0^\\infty e^{-x^2} dx = \\frac{\\sqrt{\\pi}}{2}");
    // mathDemo2.setSize(56);
    // mathDemo2.setPosition(Position::CENTER);
    // mathDemo2.setColor("#FFFFFF");
    // mathDemo2.show(0.5f);
    //
    // window.wait(5.0f);
    // mathDemo2.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 1.9 Text stroke
    // window.wait(0.5f);
    // cmdLabel = window.setText("setStroke(4.0f) + setStrokeColor(\"#000000\")");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto strokeDemo = window.setText("Outlined Text");
    // strokeDemo.setSize(72);
    // strokeDemo.setPosition(Position::CENTER);
    // strokeDemo.setColor("#FFFFFF");
    // strokeDemo.setStroke(4.0f);
    // strokeDemo.setStrokeColor("#000000");
    // strokeDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // strokeDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 1.10 Text gradient
    // window.wait(0.5f);
    // cmdLabel = window.setText("setGradient(\"#FF0000\", \"#0000FF\", 45.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto gradDemo = window.setText("Gradient Text");
    // gradDemo.setSize(72);
    // gradDemo.setPosition(Position::CENTER);
    // gradDemo.setGradient("#FF0000", "#0000FF", 45.0f);
    // gradDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // gradDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 1.11 Multiline/paragraph
    // window.wait(0.5f);
    // cmdLabel = window.setText("setLineHeight(1.5f) + setMaxWidth(600)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto paraDemo = window.setText("This is a multi-line paragraph that demonstrates word wrapping. When the text exceeds the maximum width, it automatically wraps to the next line with proper line height spacing.");
    // paraDemo.setSize(32);
    // paraDemo.setPosition(Position::CENTER);
    // paraDemo.setColor("#FFFFFF");
    // paraDemo.setLineHeight(1.5f);
    // paraDemo.setMaxWidth(600);
    // paraDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // paraDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    window.wait(5.0f);
    window.clear();
    //
    // // ============================================================================
    // // SECTION 2: ANIMATIONS - FADE
    // // ============================================================================
    //
    // sectionTitle = window.setText("Section 2: Animations - Fade");
    // sectionTitle.setPosition(Position::TLEFT);
    // sectionTitle.setSize(36);
    // sectionTitle.setColor("#FFFFFF");
    // sectionTitle.show(0.5f);
    //
    // // 2.1 Fade in
    // window.wait(5.0f);
    // cmdLabel = window.setText("show(1.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto fadeIn = window.setText("Fading In...");
    // fadeIn.setSize(56);
    // fadeIn.setPosition(Position::CENTER);
    // fadeIn.setColor("#FF5733");
    // fadeIn.show(1.0f);
    //
    // window.wait(5.0f);
    // fadeIn.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 2.2 Fade out
    // window.wait(0.5f);
    // cmdLabel = window.setText("hide(1.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto fadeOut = window.setText("Fading Out...");
    // fadeOut.setSize(56);
    // fadeOut.setPosition(Position::CENTER);
    // fadeOut.setColor("#33FF57");
    // fadeOut.show(0.3f);
    //
    // window.wait(5.0f);
    // fadeOut.hide(1.0f);
    //
    // window.wait(5.0f);
    // cmdLabel.hide(0.3f);
    //
    // // 2.3 Fade in from direction
    // window.wait(0.5f);
    // cmdLabel = window.setText("show(1.0f, Direction::UP)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto fadeInDir = window.setText("Sliding Up!");
    // fadeInDir.setSize(56);
    // fadeInDir.setPosition(Position::CENTER);
    // fadeInDir.setColor("#5733FF");
    // fadeInDir.show(1.0f, Direction::UP);
    //
    // window.wait(5.0f);
    // fadeInDir.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 2.4 Fade out to direction
    // window.wait(0.5f);
    // cmdLabel = window.setText("hide(1.0f, Direction::RIGHT)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto fadeOutDir = window.setText("Sliding Right!");
    // fadeOutDir.setSize(56);
    // fadeOutDir.setPosition(Position::CENTER);
    // fadeOutDir.setColor("#FFFF00");
    // fadeOutDir.show(0.3f);
    //
    // window.wait(5.0f);
    // fadeOutDir.hide(1.0f, Direction::RIGHT);
    //
    // window.wait(5.0f);
    // cmdLabel.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    // window.wait(5.0f);
    // window.clear();
    //
    // // ============================================================================
    // // SECTION 3: ANIMATIONS - TRANSFORM
    // // ============================================================================
    //
    // sectionTitle = window.setText("Section 3: Animations - Transform");
    // sectionTitle.setPosition(Position::TLEFT);
    // sectionTitle.setSize(36);
    // sectionTitle.setColor("#FFFFFF");
    // sectionTitle.show(0.5f);
    //
    // // 3.1 MoveTo
    // window.wait(5.0f);
    // cmdLabel = window.setText("MoveTo(1.5f, 1200, 600)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto moveShape = window.setCircle(60);
    // moveShape.setPosition(300, 400);
    // moveShape.setFill("#FF5733");
    // moveShape.show(0.5f);
    //
    // window.wait(5.0f);
    // moveShape.MoveTo(1.5f, 1200, 600);
    //
    // window.wait(5.0f);
    // moveShape.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 3.2 Scale
    // window.wait(0.5f);
    // cmdLabel = window.setText("Scale(1.0f, 2.5f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto scaleShape = window.setRectangle(100, 100);
    // scaleShape.setPosition(Position::CENTER);
    // scaleShape.setFill("#33FF57");
    // scaleShape.show(0.5f);
    //
    // window.wait(5.0f);
    // scaleShape.Scale(1.0f, 2.5f);
    //
    // window.wait(5.0f);
    // scaleShape.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 3.3 Rotate
    // window.wait(0.5f);
    // cmdLabel = window.setText("Rotate(1.5f, 360.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto rotateShape = window.setTriangle(120);
    // rotateShape.setPosition(Position::CENTER);
    // rotateShape.setFill("#5733FF");
    // rotateShape.show(0.5f);
    //
    // window.wait(5.0f);
    // rotateShape.Rotate(1.5f, 360.0f);
    //
    // window.wait(5.0f);
    // rotateShape.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 3.4 Morph (Shape)
    // window.wait(0.5f);
    // cmdLabel = window.setText("morphTo(targetShape, 2.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto morphSrc = window.setCircle(80);
    // morphSrc.setPosition(Position::CENTER);
    // morphSrc.setFill("#FF5733");
    // morphSrc.show(0.5f);
    //
    // auto morphTgt = window.setRectangle(200, 120);
    // morphTgt.setPosition(Position::CENTER);
    // morphTgt.setFill("#33FF57");
    //
    // window.wait(5.0f);
    // morphSrc.morphTo(morphTgt, 2.0f);
    //
    // window.wait(5.0f);
    // morphTgt.hide(0.5f);
    // cmdLabel.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    // window.wait(5.0f);
    // window.clear();
    //
    // // ============================================================================
    // // SECTION 4: ANIMATIONS - SHOWING
    // // ============================================================================
    //
    // sectionTitle = window.setText("Section 4: Animations - Showing");
    // sectionTitle.setPosition(Position::TLEFT);
    // sectionTitle.setSize(36);
    // sectionTitle.setColor("#FFFFFF");
    // sectionTitle.show(0.5f);
    //
    // // 4.1 Write (typewriter)
    // window.wait(5.0f);
    // cmdLabel = window.setText("Write(2.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto writeDemo = window.setText("Typewriter Effect!");
    // writeDemo.setSize(56);
    // writeDemo.setPosition(Position::CENTER);
    // writeDemo.setColor("#FF5733");
    // writeDemo.Write(2.0f);
    //
    // window.wait(5.0f);
    // writeDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 4.2 AddLetterByLetter
    // window.wait(0.5f);
    // cmdLabel = window.setText("AddLetterByLetter(3.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto letterDemo = window.setText("Letter by Letter");
    // letterDemo.setSize(56);
    // letterDemo.setPosition(Position::CENTER);
    // letterDemo.setColor("#33FF57");
    // letterDemo.AddLetterByLetter(3.0f);
    //
    // window.wait(5.0f);
    // letterDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 4.3 DrawBorderThenFill
    // window.wait(0.5f);
    // cmdLabel = window.setText("DrawBorderThenFill(2.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto borderFillDemo = window.setText("Border Then Fill");
    // borderFillDemo.setSize(56);
    // borderFillDemo.setPosition(Position::CENTER);
    // borderFillDemo.setColor("#5733FF");
    // borderFillDemo.setStroke(3.0f);
    // borderFillDemo.setStrokeColor("#FFFFFF");
    // borderFillDemo.DrawBorderThenFill(2.0f);
    //
    // window.wait(5.0f);
    // borderFillDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    // window.wait(5.0f);
    // window.clear();
    //
    // // ============================================================================
    // // SECTION 5: ANIMATIONS - EMPHASIS
    // // ============================================================================
    //
    // sectionTitle = window.setText("Section 5: Animations - Emphasis");
    // sectionTitle.setPosition(Position::TLEFT);
    // sectionTitle.setSize(36);
    // sectionTitle.setColor("#FFFFFF");
    // sectionTitle.show(0.5f);
    //
    // // 5.1 Indicate
    // window.wait(5.0f);
    // cmdLabel = window.setText("Indicate(1.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto indicateDemo = window.setText("Indicate Me!");
    // indicateDemo.setSize(56);
    // indicateDemo.setPosition(Position::CENTER);
    // indicateDemo.setColor("#FF5733");
    // indicateDemo.show(0.3f);
    //
    // window.wait(5.0f);
    // indicateDemo.Indicate(1.0f);
    //
    // window.wait(5.0f);
    // indicateDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 5.2 Flash
    // window.wait(0.5f);
    // cmdLabel = window.setText("Flash(0.5f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto flashDemo = window.setText("Flash!");
    // flashDemo.setSize(56);
    // flashDemo.setPosition(Position::CENTER);
    // flashDemo.setColor("#33FF57");
    // flashDemo.show(0.3f);
    //
    // window.wait(5.0f);
    // flashDemo.Flash(0.5f);
    //
    // window.wait(5.0f);
    // flashDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 5.3 Circumscribe
    // window.wait(0.5f);
    // cmdLabel = window.setText("Circumscribe(1.5f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto circumDemo = window.setText("Circumscribe");
    // circumDemo.setSize(56);
    // circumDemo.setPosition(Position::CENTER);
    // circumDemo.setColor("#5733FF");
    // circumDemo.show(0.3f);
    //
    // window.wait(5.0f);
    // circumDemo.Circumscribe(1.5f);
    //
    // window.wait(5.0f);
    // circumDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 5.4 Wiggle
    // window.wait(0.5f);
    // cmdLabel = window.setText("Wiggle(1.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto wiggleDemo = window.setText("Wiggle!");
    // wiggleDemo.setSize(56);
    // wiggleDemo.setPosition(Position::CENTER);
    // wiggleDemo.setColor("#FFFF00");
    // wiggleDemo.show(0.3f);
    //
    // window.wait(5.0f);
    // wiggleDemo.Wiggle(1.0f);
    //
    // window.wait(5.0f);
    // wiggleDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 5.5 Pulse
    // window.wait(0.5f);
    // cmdLabel = window.setText("Pulse(1.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto pulseDemo = window.setText("Pulse!");
    // pulseDemo.setSize(56);
    // pulseDemo.setPosition(Position::CENTER);
    // pulseDemo.setColor("#00FFFF");
    // pulseDemo.show(0.3f);
    //
    // window.wait(5.0f);
    // pulseDemo.Pulse(1.0f);
    //
    // window.wait(5.0f);
    // pulseDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 5.6 FocusOn
    // window.wait(0.5f);
    // cmdLabel = window.setText("FocusOn(1.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto focusDemo = window.setText("Focus On Me!");
    // focusDemo.setSize(56);
    // focusDemo.setPosition(Position::CENTER);
    // focusDemo.setColor("#FF00FF");
    // focusDemo.show(0.3f);
    //
    // window.wait(5.0f);
    // focusDemo.FocusOn(1.0f);
    //
    // window.wait(5.0f);
    // focusDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    // window.wait(5.0f);
    // window.clear();
    //
    // // ============================================================================
    // // SECTION 6: ANIMATIONS - MOVEMENT
    // // ============================================================================
    //
    // sectionTitle = window.setText("Section 6: Animations - Movement");
    // sectionTitle.setPosition(Position::TLEFT);
    // sectionTitle.setSize(36);
    // sectionTitle.setColor("#FFFFFF");
    // sectionTitle.show(0.5f);
    //
    // // 6.1 SpiralIn
    // window.wait(5.0f);
    // cmdLabel = window.setText("SpiralIn(2.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto spiralInDemo = window.setCircle(40);
    // spiralInDemo.setPosition(Position::CENTER);
    // spiralInDemo.setFill("#FF5733");
    // spiralInDemo.show(0.3f);
    // spiralInDemo.SpiralIn(2.0f);
    //
    // window.wait(5.0f);
    // spiralInDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 6.2 SpiralOut
    // window.wait(0.5f);
    // cmdLabel = window.setText("SpiralOut(2.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto spiralOutDemo = window.setCircle(40);
    // spiralOutDemo.setPosition(Position::CENTER);
    // spiralOutDemo.setFill("#33FF57");
    // spiralOutDemo.show(0.3f);
    //
    // window.wait(5.0f);
    // spiralOutDemo.SpiralOut(2.0f);
    //
    // window.wait(5.0f);
    // cmdLabel.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    // window.wait(5.0f);
    // window.clear();
    //
    // // ============================================================================
    // // SECTION 7: SHAPES - BASIC
    // // ============================================================================
    //
    // sectionTitle = window.setText("Section 7: Shapes - Basic");
    // sectionTitle.setPosition(Position::TLEFT);
    // sectionTitle.setSize(36);
    // sectionTitle.setColor("#FFFFFF");
    // sectionTitle.show(0.5f);
    //
    // // 7.1 Circle
    // window.wait(5.0f);
    // cmdLabel = window.setText("setCircle(80)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto circleDemo = window.setCircle(80);
    // circleDemo.setPosition(Position::CENTER);
    // circleDemo.setFill("#FF5733");
    // circleDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // circleDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.2 Rectangle
    // window.wait(0.5f);
    // cmdLabel = window.setText("setRectangle(200, 100)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto rectDemo = window.setRectangle(200, 100);
    // rectDemo.setPosition(Position::CENTER);
    // rectDemo.setFill("#33FF57");
    // rectDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // rectDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.3 Triangle
    // window.wait(0.5f);
    // cmdLabel = window.setText("setTriangle(120)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto triDemo = window.setTriangle(120);
    // triDemo.setPosition(Position::CENTER);
    // triDemo.setFill("#5733FF");
    // triDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // triDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.4 Line
    // window.wait(0.5f);
    // cmdLabel = window.setText("setLine(400, 400, 1500, 700)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto lineDemo = window.setLine(400, 400, 1500, 700);
    // lineDemo.setFill("#FFFF00");
    // lineDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // lineDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.5 Arrow
    // window.wait(0.5f);
    // cmdLabel = window.setText("setArrow(400, 540, 1500, 540)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto arrowDemo = window.setArrow(400, 540, 1500, 540);
    // arrowDemo.setFill("#00FFFF");
    // arrowDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // arrowDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.6 Double Arrow
    // window.wait(0.5f);
    // cmdLabel = window.setText("setDoubleArrow(400, 540, 1500, 540)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto dblArrowDemo = window.setDoubleArrow(400, 540, 1500, 540);
    // dblArrowDemo.setFill("#FF00FF");
    // dblArrowDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // dblArrowDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.7 Regular Polygon (Hexagon)
    // window.wait(0.5f);
    // cmdLabel = window.setText("setRegularPolygon(6, 80)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto hexDemo = window.setRegularPolygon(6, 80);
    // hexDemo.setPosition(Position::CENTER);
    // hexDemo.setFill("#FF8800");
    // hexDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // hexDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.8 Ellipse
    // window.wait(0.5f);
    // cmdLabel = window.setText("setEllipse(120, 60)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto ellipseDemo = window.setEllipse(120, 60);
    // ellipseDemo.setPosition(Position::CENTER);
    // ellipseDemo.setFill("#88FF00");
    // ellipseDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // ellipseDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.9 Arc
    // window.wait(0.5f);
    // cmdLabel = window.setText("setArc(80, 0, 270)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto arcDemo = window.setArc(80, 0, 270);
    // arcDemo.setPosition(Position::CENTER);
    // arcDemo.setFill("#0088FF");
    // arcDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // arcDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.10 Dot
    // window.wait(0.5f);
    // cmdLabel = window.setText("setDot(20)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto dotDemo = window.setDot(20);
    // dotDemo.setPosition(Position::CENTER);
    // dotDemo.setFill("#FF0088");
    // dotDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // dotDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.11 Star
    // window.wait(0.5f);
    // cmdLabel = window.setText("setStar(5, 80, 40)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto starDemo = window.setStar(5, 80, 40);
    // starDemo.setPosition(Position::CENTER);
    // starDemo.setFill("#FFFF00");
    // starDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // starDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 7.12 Rounded Rectangle
    // window.wait(0.5f);
    // cmdLabel = window.setText("setRoundedRectangle(200, 100, 20)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto roundRectDemo = window.setRoundedRectangle(200, 100, 20);
    // roundRectDemo.setPosition(Position::CENTER);
    // roundRectDemo.setFill("#00FF88");
    // roundRectDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // roundRectDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    // window.wait(5.0f);
    // window.clear();
    //
    // // ============================================================================
    // // SECTION 8: SHAPES - ADVANCED
    // // ============================================================================
    //
    // sectionTitle = window.setText("Section 8: Shapes - Advanced");
    // sectionTitle.setPosition(Position::TLEFT);
    // sectionTitle.setSize(36);
    // sectionTitle.setColor("#FFFFFF");
    // sectionTitle.show(0.5f);
    //
    // // 8.1 Axes
    // window.wait(5.0f);
    // cmdLabel = window.setText("setAxes(-5, 5, -3, 3)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto axesDemo = window.setAxes(-5.0f, 5.0f, -3.0f, 3.0f);
    // axesDemo.setTickSpacing(1.0f, 1.0f);
    // axesDemo.setColor("#FFFFFF");
    // axesDemo.show(1.0f);
    //
    // window.wait(5.0f);
    // axesDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 8.2 Graph (function)
    // window.wait(0.5f);
    // cmdLabel = window.setText("setGraph(axes, sin, 200)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto axesGraph = window.setAxes(-5.0f, 5.0f, -2.0f, 2.0f);
    // axesGraph.setColor("#666666");
    // axesGraph.show(0.5f);
    //
    // auto graphFunc = window.setGraph(axesGraph, [](float x) { return std::sin(x); }, 200);
    // graphFunc.setColor("#FF5733");
    // graphFunc.setThickness(3.0f);
    // graphFunc.show(1.5f);
    //
    // window.wait(5.0f);
    // axesGraph.hide(0.5f);
    // graphFunc.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 8.3 Table
    // window.wait(0.5f);
    // cmdLabel = window.setText("setTable(data)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // std::vector<std::vector<std::string>> tableData = {
    //     {"Name", "Score", "Grade"},
    //     {"Alice", "95", "A"},
    //     {"Bob", "87", "B"},
    //     {"Carol", "92", "A"}
    // };
    //
    // auto tableDemo = window.setTable(tableData);
    // tableDemo.setPosition(Position::CENTER);
    // tableDemo.setCellSize(120, 45);
    // tableDemo.setFontSize(20);
    // tableDemo.setTextColor("#FFFFFF");
    // tableDemo.setGridColor("#555555");
    // tableDemo.setHeaderColor("#2266AA");
    // tableDemo.show(1.0f);
    //
    // window.wait(5.0f);
    // tableDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    // window.wait(5.0f);
    // window.clear();

    // ============================================================================
    // SECTION 9: SCENE & CAMERA
    // ============================================================================

    // sectionTitle = window.setText("Section 9: Scene & Camera");
    // sectionTitle.setPosition(Position::TLEFT);
    // sectionTitle.setSize(36);
    // sectionTitle.setColor("#FFFFFF");
    // sectionTitle.show(0.5f);
    //
    // // Create reference objects for camera demos
    // auto camCircle = window.setCircle(60);
    // camCircle.setPosition(Position::CENTER);
    // camCircle.setFill("#FF5733");
    // camCircle.show(0.3f);
    //
    // auto camC1 = window.setCircle(40);
    // camC1.setPosition(Position::TLEFT);
    // camC1.setFill("#33FF57");
    // camC1.show(0.3f);
    //
    // auto camC2 = window.setCircle(40);
    // camC2.setPosition(Position::TRIGHT);
    // camC2.setFill("#5733FF");
    // camC2.show(0.3f);
    //
    // auto camC3 = window.setCircle(40);
    // camC3.setPosition(Position::BLEFT);
    // camC3.setFill("#FFFF00");
    // camC3.show(0.3f);
    //
    // auto camC4 = window.setCircle(40);
    // camC4.setPosition(Position::BRIGHT);
    // camC4.setFill("#00FFFF");
    // camC4.show(0.3f);
    //
    // // 9.1 Camera zoom
    // window.wait(5.0f);
    // cmdLabel = window.setText("setCameraZoom(1.8f, 1.5f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // window.setCameraZoom(1.8f, 1.5f);
    //
    // window.wait(5.0f);
    // cmdLabel.hide(0.3f);
    //
    //
    // // 9.2 Camera pan
    // window.wait(0.5f);
    // window.resetCamera(1.5f);
    // cmdLabel = window.setText("setCameraPan(0.3f, 0.0f, 1.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // window.setCameraPan(0.3f, 0.0f, 1.0f);
    //
    // window.wait(5.0f);
    // cmdLabel.hide(0.3f);
    //
    // // 9.3 Camera rotate
    // window.wait(0.5f);
    // cmdLabel = window.setText("setCameraRotate(25.0f, 1.5f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // window.setCameraRotate(25.0f, 1.5f);
    //
    // window.wait(5.0f);
    // cmdLabel.hide(0.3f);
    //
    // // 9.4 Camera reset
    // window.wait(0.5f);
    // cmdLabel = window.setText("resetCamera(1.5f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // window.resetCamera(1.5f);
    //
    // window.wait(5.0f);
    // cmdLabel.hide(0.3f);
    //
    // camCircle.hide(0.3f);
    // camC1.hide(0.3f);
    // camC2.hide(0.3f);
    // camC3.hide(0.3f);
    // camC4.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    // window.wait(5.0f);
    // window.clear();
    //
    // // ============================================================================
    // // SECTION 10: 3D FEATURES
    // // ============================================================================
    //
    // sectionTitle = window.setText("Section 10: 3D Features");
    // sectionTitle.setPosition(Position::TLEFT);
    // sectionTitle.setSize(36);
    // sectionTitle.setColor("#FFFFFF");
    // sectionTitle.show(0.5f);
    //
    // // 10.1 Enable 3D mode
    // window.wait(5.0f);
    // cmdLabel = window.setText("set3DMode(true)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // window.set3DMode(true);
    // window.setCamera3D(5.0f, 3.0f, 5.0f, 0.0f, 0.0f, 0.0f);
    // // Orbit the camera 360° around the scene for the full 3D demo section.
    // {
    //     constexpr float kTwoPi = 6.283185307179586f;
    //     constexpr float kOrbitDuration = 43.5f;
    //     float dx = 5.0f, dy = 3.0f, dz = 5.0f;
    //     float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    //     float phi = std::acos(dy / distance);
    //     float theta = std::atan2(dz, dx);
    //     window.orbitCamera(theta + kTwoPi, phi, distance, kOrbitDuration);
    // }
    //
    // window.wait(5.0f);
    // cmdLabel.hide(0.3f);
    //
    // // 10.2 3D Axes
    // window.wait(0.5f);
    // cmdLabel = window.setText("setAxes3D(-2, 2, -2, 2, -2, 2)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto axes3DDemo = window.setAxes3D(-2.0f, 2.0f, -2.0f, 2.0f, -2.0f, 2.0f);
    // axes3DDemo.setColor("#FFFFFF");
    // axes3DDemo.show(1.0f);
    //
    // window.wait(5.0f);
    // cmdLabel.hide(0.3f);
    //
    // // 10.3 Sphere
    // window.wait(0.5f);
    // cmdLabel = window.setText("setSphere(0.5f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto sphereDemo = window.setSphere(0.5f);
    // sphereDemo.setPosition(0.0f, 1.0f, 0.0f);
    // sphereDemo.setColor("#FF5733");
    // sphereDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // sphereDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 10.4 Cube
    // window.wait(0.5f);
    // cmdLabel = window.setText("setCube3D(0.8f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto cubeDemo = window.setCube3D(0.8f);
    // cubeDemo.setPosition(-1.0f, 0.4f, 0.0f);
    // cubeDemo.setColor("#33FF57");
    // cubeDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // cubeDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 10.5 Cylinder
    // window.wait(0.5f);
    // cmdLabel = window.setText("setCylinder(0.3f, 1.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto cylinderDemo = window.setCylinder(0.3f, 1.0f);
    // cylinderDemo.setPosition(1.0f, 0.5f, 0.0f);
    // cylinderDemo.setColor("#5733FF");
    // cylinderDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // cylinderDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 10.6 Cone
    // window.wait(0.5f);
    // cmdLabel = window.setText("setCone(0.4f, 1.0f)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto coneDemo = window.setCone(0.4f, 1.0f);
    // coneDemo.setPosition(0.0f, 0.5f, 1.0f);
    // coneDemo.setColor("#FFFF00");
    // coneDemo.show(0.5f);
    //
    // window.wait(5.0f);
    // coneDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 10.7 Surface
    // window.wait(0.5f);
    // cmdLabel = window.setText("setSurface(sin(x)*cos(y), -2, 2, -2, 2)");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // auto surfaceDemo = window.setSurface(
    //     [](float x, float y) { return std::sin(x) * std::cos(y) * 0.5f; },
    //     -2.0f, 2.0f, -2.0f, 2.0f
    // );
    // surfaceDemo.setColor("#00FFFF");
    // surfaceDemo.show(1.0f);
    //
    // window.wait(5.0f);
    // surfaceDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    //
    // // 10.8 Orbit camera
    // window.wait(0.5f);
    // cmdLabel = window.setText("Camera orbit: 360°");
    // cmdLabel.setPosition(Position::TRIGHT);
    // cmdLabel.setSize(24);
    // cmdLabel.setColor("#00FF00");
    // cmdLabel.show(0.3f);
    //
    // // Show some objects for camera demo
    // auto orbitSphere = window.setSphere(0.6f);
    // orbitSphere.setPosition(0.0f, 0.0f, 0.0f);
    // orbitSphere.setColor("#FF5733");
    // orbitSphere.show(0.3f);
    //
    // window.wait(5.0f);
    // orbitSphere.hide(0.5f);
    // axes3DDemo.hide(0.5f);
    // cmdLabel.hide(0.3f);
    // // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this
    //
    // window.set3DMode(false);
    // window.wait(5.0f);
    // window.clear();

    // ============================================================================
    // SECTION 11: TIMING & CONTROL
    // ============================================================================

    sectionTitle = window.setText("Section 11: Timing & Control");
    sectionTitle.setPosition(Position::TLEFT);
    sectionTitle.setSize(36);
    sectionTitle.setColor("#FFFFFF");
    sectionTitle.show(0.5f);

    // 11.1 Easing functions
    window.wait(5.0f);
    cmdLabel = window.setText("setEasing(Easing::EaseOutBounce)");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto easeDemo = window.setCircle(50);
    easeDemo.setPosition(200, 540);
    easeDemo.setFill("#FF5733");
    easeDemo.setEasing(Easing::EaseOutBounce);
    easeDemo.show(0.3f);

    window.wait(0.5f);
    easeDemo.MoveTo(2.0f, 1700, 540);

    window.wait(5.0f);
    easeDemo.hide(0.5f);
    cmdLabel.hide(0.3f);

    // 11.2 Delay
    window.wait(0.5f);
    cmdLabel = window.setText("delay(1.5f).show(0.5f)");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto delayDemo = window.setText("Delayed by 1.5 seconds!");
    delayDemo.setSize(48);
    delayDemo.setPosition(Position::CENTER);
    delayDemo.setColor("#33FF57");
    delayDemo.delay(1.5f);
    delayDemo.show(0.5f);

    window.wait(5.0f);
    delayDemo.hide(0.5f);
    cmdLabel.hide(0.3f);

    // 11.3 Chaining with then()
    window.wait(0.5f);
    cmdLabel = window.setText("show().then().MoveTo().then().Scale()");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto chainDemo = window.setCircle(40);
    chainDemo.setPosition(300, 540);
    chainDemo.setFill("#5733FF");
    chainDemo.show(0.5f);
    chainDemo.then();
    chainDemo.MoveTo(1.0f, 960, 540);
    chainDemo.then();
    chainDemo.Scale(0.5f, 2.0f);

    window.wait(5.0f);
    chainDemo.hide(0.5f);
    cmdLabel.hide(0.3f);

    // 11.4 Looping with repeat
    window.wait(0.5f);
    cmdLabel = window.setText("repeat(3).Rotate(0.5f, 360)");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto loopDemo = window.setTriangle(80);
    loopDemo.setPosition(Position::CENTER);
    loopDemo.setFill("#FFFF00");
    loopDemo.show(0.3f);

    window.wait(0.5f);
    loopDemo.repeat(3);
    loopDemo.Rotate(0.5f, 360.0f);

    window.wait(5.0f);
    loopDemo.stopLoop();
    loopDemo.hide(0.5f);
    cmdLabel.hide(0.3f);

    // 11.5 AnimationGroup (parallel)
    window.wait(0.5f);
    cmdLabel = window.setText("createAnimationGroup().addShow().play()");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto groupCircle1 = window.setCircle(40);
    groupCircle1.setPosition(400, 400);
    groupCircle1.setFill("#FF5733");

    auto groupCircle2 = window.setCircle(40);
    groupCircle2.setPosition(960, 400);
    groupCircle2.setFill("#33FF57");

    auto groupCircle3 = window.setCircle(40);
    groupCircle3.setPosition(1520, 400);
    groupCircle3.setFill("#5733FF");

    auto animGroup = window.createAnimationGroup();
    animGroup.addShow(groupCircle1, 1.0f);
    animGroup.addShow(groupCircle2, 1.0f);
    animGroup.addShow(groupCircle3, 1.0f);
    animGroup.play();

    window.wait(5.0f);
    groupCircle1.hide(0.5f);
    groupCircle2.hide(0.5f);
    groupCircle3.hide(0.5f);
    cmdLabel.hide(0.3f);
    // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this

    window.wait(5.0f);
    window.clear();

    // ============================================================================
    // SECTION 12: GROUPING & HIERARCHY
    // ============================================================================

    sectionTitle = window.setText("Section 12: Grouping & Hierarchy");
    sectionTitle.setPosition(Position::TLEFT);
    sectionTitle.setSize(36);
    sectionTitle.setColor("#FFFFFF");
    sectionTitle.show(0.5f);

    // 12.1 Group objects
    window.wait(5.0f);
    cmdLabel = window.setText("createGroup().add(elem1).add(elem2)");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto grpCircle = window.setCircle(40);
    grpCircle.setFill("#FF5733");

    auto grpLabel = window.setText("Grouped!");
    grpLabel.setSize(24);
    grpLabel.setColor("#FFFFFF");

    auto grp = window.createGroup();
    grp.add(grpCircle).add(grpLabel);
    grp.setPosition(Position::CENTER);
    grp.show(0.5f);

    window.wait(5.0f);
    grp.hide(0.5f);
    cmdLabel.hide(0.3f);

    // 12.2 Arrange row
    window.wait(0.5f);
    cmdLabel = window.setText("arrange(Direction::RIGHT, 30.0f)");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto rowGrp = window.createGroup();
    for (int i = 0; i < 5; i++) {
        auto c = window.setCircle(35);
        c.setFill(i % 2 == 0 ? "#FF5733" : "#33FF57");
        rowGrp.add(c);
    }
    rowGrp.arrange(Direction::RIGHT, 30.0f);
    rowGrp.setPosition(Position::CENTER);
    rowGrp.show(0.5f);

    window.wait(5.0f);
    rowGrp.hide(0.5f);
    cmdLabel.hide(0.3f);

    // 12.3 Arrange column
    window.wait(0.5f);
    cmdLabel = window.setText("arrange(Direction::DOWN, 20.0f)");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto colGrp = window.createGroup();
    for (int i = 0; i < 4; i++) {
        auto r = window.setRectangle(120, 40);
        r.setFill(i % 2 == 0 ? "#5733FF" : "#FFFF00");
        colGrp.add(r);
    }
    colGrp.arrange(Direction::DOWN, 20.0f);
    colGrp.setPosition(Position::CENTER);
    colGrp.show(0.5f);

    window.wait(5.0f);
    colGrp.hide(0.5f);
    cmdLabel.hide(0.3f);

    // 12.4 Arrange grid
    window.wait(0.5f);
    cmdLabel = window.setText("arrangeInGrid(3, 15.0f, 15.0f)");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto gridGrp = window.createGroup();
    for (int i = 0; i < 9; i++) {
        auto cell = window.setRectangle(80, 80);
        cell.setFill(i % 3 == 0 ? "#FF5733" : (i % 3 == 1 ? "#33FF57" : "#5733FF"));
        gridGrp.add(cell);
    }
    gridGrp.arrangeInGrid(3, 15.0f, 15.0f);
    gridGrp.setPosition(Position::CENTER);
    gridGrp.show(0.5f);

    window.wait(5.0f);
    gridGrp.hide(0.5f);
    cmdLabel.hide(0.3f);

    // 12.5 Z-ordering
    window.wait(0.5f);
    cmdLabel = window.setText("setZIndex(0) vs setZIndex(1)");
    cmdLabel.setPosition(Position::TRIGHT);
    cmdLabel.setSize(24);
    cmdLabel.setColor("#00FF00");
    cmdLabel.show(0.3f);

    auto backRect = window.setRectangle(300, 200);
    backRect.setPosition(Position::CENTER);
    backRect.setFill("#333333");
    backRect.setZIndex(0);
    backRect.show(0.3f);

    auto frontText = window.setText("Z-Index: On Top!");
    frontText.setSize(32);
    frontText.setPosition(Position::CENTER);
    frontText.setColor("#FFFFFF");
    frontText.setZIndex(1);
    frontText.show(0.3f);

    window.wait(5.0f);
    backRect.hide(0.5f);
    frontText.hide(0.5f);
    cmdLabel.hide(0.3f);
    // sectionTitle.hide(0.3f);  // Removed - window.clear() handles this

    window.wait(5.0f);
    window.clear();

    // ============================================================================
    // DEMO COMPLETE
    // ============================================================================

    window.wait(5.0f);
    auto finalText = window.setText("Feature Demo Complete!");
    finalText.setSize(72);
    finalText.setPosition(Position::CENTER);
    finalText.setColor("#00FFAA");
    finalText.show(0.5f);

    auto subText = window.setText("All 83 features demonstrated across 12 sections");
    subText.setSize(32);
    subText.setPosition(960, 620);
    subText.setColor("#AAAAAA");
    subText.show(0.5f, Direction::UP);

    window.run();
    return 0;
}

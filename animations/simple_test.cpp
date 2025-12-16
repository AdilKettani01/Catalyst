// Simple text rendering test
#include "../catalyst.h"

int main() {
    Catalyst window(1920, 1080);
    window.setBackground("#1a1a2e");

    auto text = window.setText("Hello World");
    text.setSize(72);
    text.setPosition(Position::CENTER);
    text.setColor("#FFFFFF");
    text.show(0.5f);  // Should appear at time 0

    window.run();
    return 0;
}

#include <Catalyst>

int main() {
    Catalyst window(1280, 720);
    window.setBackground("#1a1a2e");

    auto title = window.setText("Hello from an external project!");
    title.setPosition(Position::CENTER);
    title.setSize(48);
    title.setColor("#FFFFFF");
    title.show(0.5f);

    window.run();
    return 0;
}


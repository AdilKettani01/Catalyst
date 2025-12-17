#include <Catalyst>

int main() {
    Catalyst window(1280, 720);
    window.setBackground("#1a1a2e");

    auto a = window.setText("A");
    auto b = window.setText("B");
    auto c = window.setText("C");

    a.move_to(420, 260);
    b.next_to(a, Direction::RIGHT, 80.0f);
    c.next_to(b, Direction::RIGHT, 80.0f);

    a.show(0.5f);
    b.show(0.5f);
    c.show(0.5f);

    auto group = window.createGroup();
    group.add(a).add(b).add(c);
    group.to_edge(Direction::UP, 60.0f);

    group[0].setColor("#FF5555");
    group[-1].setColor(100, 200, 255);
    group[Slice::between(1, 3)].shift(Direction::DOWN, 80.0f);
    group[Slice::between(1, 3)].MoveTo(1.0f, 640, 360);

    auto formula = window.setMath("c = 0.0000", {{"c", "#FF5555"}});
    formula.next_to(group, Direction::DOWN, 50.0f);
    formula.align_to(group, Direction::LEFT);
    formula.show(0.5f);
    formula.set_color_by_tex("0.0000", "#55FF55");

    window.run();
    return 0;
}
